import datetime
import pfio
import pandas as pd
import os
import argparse
import sys
import glob
import numpy as np

GAUGES_FILE = 'CONUS_AllStations_Summary_FIXED_stats.Run4P.csv'
RESOLUTION = 1000.0
MANNINGS = 5.52e-6
RM = (RESOLUTION / MANNINGS)
CMS_TO_CFS = 35.3147


def is_valid_path(parser, arg):
    if not os.path.isdir(arg):
        parser.error("The path %s does not exist!" % arg)
    else:
        return arg  # return the arg


def is_valid_file(parser, arg):
    if not os.path.isfile(arg):
        parser.error("The file %s does not exist!" % arg)
    else:
        return open(arg, 'r')  # return open file handle


def parse_args(args):
    parser = argparse.ArgumentParser('Extract the flow data from a list of gauge sites and a domain')

    parser.add_argument("--pf_outputs", "-i", dest="pf_outputs", required=True,
                        help="path to the output files from ParFlow",
                        type=lambda x: is_valid_path(parser, x))

    parser.add_argument("--out_dir", "-o", dest="out_dir", required=True,
                        help="the directory to write outputs to",
                        type=lambda x: is_valid_path(parser, x))

    parser.add_argument("--start_date", "-s", dest="start_date", required=False,
                        type=lambda x: datetime.datetime.strptime(x, '%m-%d-%Y'),
                        help="the starting date for the simulation", )

    return parser.parse_args(args)


def find_pftcl_file(pf_outputs):
    pftcl_file = glob.glob(os.path.join(pf_outputs, '*.out.pftcl'))
    if len(pftcl_file) != 1:
        raise Exception(f'incorrect number of pftcl files found!\nExpected: 1, Got: {len(pftcl_file)}')
    return pftcl_file[0]


def parse_pftcl(pftcl_file, string_to_find):
    with open(pftcl_file, 'r') as pftcl:
        for cnt, line in enumerate(pftcl):
            if line.split(' ')[1] == string_to_find:
                return line.split(' ')[2].strip('\n').strip('\"')


def find_domain_extents(pf_outputs, runname):
    extents_file = glob.glob(os.path.join(pf_outputs, f'{runname}.txt'))
    if len(extents_file) != 1:
        raise Exception(f'incorrect number of extents files found!\nExpected: 1, Got: {len(extents_file)}')

    with open(extents_file[0], 'r') as extents:
        lines = extents.readlines()
        return lines[1].split('\t')


def get_runname_from_pftcl(pftcl_file_path):
    return os.path.split(pftcl_file_path)[1][:-(len('.out.pftcl'))]


def get_gauges_in_extents(conus_lower_left, conus_upper_right, usgs_gauges):
    return usgs_gauges.loc[(usgs_gauges['Final_i'] >= conus_lower_left[0]) &
                           (usgs_gauges['Final_i'] < conus_upper_right[0]) &
                           (usgs_gauges['Final_j'] >= conus_lower_left[1]) &
                           (usgs_gauges['Final_j'] < conus_upper_right[1])]


def check_mask_file_found(mask_file_path):
    if not os.path.isfile(mask_file_path):
        raise Exception(f'mask file not found at {mask_file_path}')


def get_gauges_in_mask(conus_lower_left, mask_data, gauges_in_extents):
    gauges_in_extents = gauges_in_extents.assign(mapped_i=lambda row: (row.Final_i - 1) - conus_lower_left[0],
                                                 mapped_j=lambda row: (row.Final_j - 1) - conus_lower_left[1],
                                                 get_data=lambda row: mask_data[0, row.mapped_j, row.mapped_i])
    return gauges_in_extents.loc[gauges_in_extents.get_data > 0]


def calc_slope_data(gauges_in_mask, slope_file_x, slope_file_y):
    # calculate the slope for all the inbounds gauges as slope=sqrt(x.slope^2+y.slope^2)
    slope_data_x = np.flip(pfio.pfread(slope_file_x), axis=1)
    slope_data_y = np.flip(pfio.pfread(slope_file_y), axis=1)
    return gauges_in_mask.assign(slope_x=lambda row: slope_data_x[0, row.mapped_j, row.mapped_i],
                                           slope_y=lambda row: slope_data_y[0, row.mapped_j, row.mapped_i],
                                           slope=lambda row: (row.slope_x ** 2 + row.slope_y ** 2) ** 0.5)


def get_pressure_files(pf_outputs, runname):
    pressure_files = glob.glob(os.path.join(pf_outputs, f'{runname}.out.press.*.pfb'))
    pressure_files.sort()
    return pressure_files


def calculate_flow_data(gauges_in_mask, pressure_files):
    # create a new empty dataframe to hold the flow data
    flow_data = pd.DataFrame()


    # iterate over all pressure files found and collect pressure information
    for p_file in pressure_files:
        top_layer = -1
        ts = int(p_file.split('.')[-2])
        pressure_data = np.flip(pfio.pfread(p_file), axis=1)
        # print(pressure_data[top_layer, 502, 249])
        # print(pressure_data.shape)
        p_data = gauges_in_mask.assign(pressure=lambda row: pressure_data[top_layer, row.mapped_j, row.mapped_i],
                                       timestep=ts)

        p_data = p_data.filter(['STAID', 'STANAME', 'mapped_i', 'mapped_j', 'slope', 'timestep', 'pressure'],
                               axis=1)
        flow_data = flow_data.append(p_data, sort=False, ignore_index=True)

    # do flow calculation
    flow_data = flow_data.assign(flow_cms=lambda row: RM * (row.slope.pow(.5)) * (row.pressure.pow(5 / 3) / 3600),
                                 flow_cfs=lambda row: row.flow_cms * CMS_TO_CFS)

    # handle failure of flow calculation when press is negative
    return flow_data.fillna(0)


def write_flows_to_csv(flow_data, out_dir):
    # filter and write outputs to individual csv files
    for name, group in flow_data.filter(['STAID', 'STANAME', 'timestep', 'flow_cms', 'flow_cfs',
                                         'mapped_i', 'mapped_j', 'pressure', 'slope']).groupby('STAID'):
        group.to_csv(os.path.join(out_dir, '{}.csv'.format(name)), index=False, sep='\t')


def get_flow_at_gauges(pf_outputs, out_dir, start_date=None):
    # make sure we have a pftcl file in the outputs directory
    pftcl_file = find_pftcl_file(pf_outputs)
    # parse the pftcl file's path and name to identify the ParFlow runname
    runname = get_runname_from_pftcl(pftcl_file)
    # get domain extents
    domain_extents = find_domain_extents(pf_outputs, runname)
    conus_lower_left = (int(domain_extents[2]), int(domain_extents[0]))
    conus_upper_right = (int(domain_extents[3]), int(domain_extents[1]))
    # find the mask file and read it
    check_mask_file_found(os.path.join(pf_outputs, f'{runname}.out.mask.pfb'))
    mask_data = np.flip(pfio.pfread(os.path.join(pf_outputs, f'{runname}.out.mask.pfb')), axis=1)
    # find the slope file names in the pftcl file
    slope_file_x = parse_pftcl(pftcl_file, 'TopoSlopesX.FileName')
    slope_file_y = parse_pftcl(pftcl_file, 'TopoSlopesY.FileName')
    # read the list of gauges from the csv file
    usgs_gauges = pd.read_csv(GAUGES_FILE)
    # determine which gauges are inside the bounds of our domain
    # filter the gauges by those that are inside our masked domain
    gauges_in_extents = get_gauges_in_extents(conus_lower_left, conus_upper_right, usgs_gauges)
    gauges_in_mask = get_gauges_in_mask(conus_lower_left, mask_data, gauges_in_extents)
    # debugging
    # print(gauges_in_extents)
    # print(gauges_in_mask)
    if not gauges_in_mask.empty:
        gauges_in_mask = calc_slope_data(gauges_in_mask,
                                         os.path.join(pf_outputs, slope_file_x),
                                         os.path.join(pf_outputs, slope_file_y))
        pressure_files = get_pressure_files(pf_outputs, runname)
        flow_data = calculate_flow_data(gauges_in_mask, pressure_files)
        # debugging
        # print(flow_data)
        # Given optional start_date argument, set appropriate start date
        if not start_date is None:
            flow_data['timestep'] = pd.to_datetime(flow_data.timestep, unit='D', origin=pd.Timestamp(start_date))

        return flow_data


def generate_flow_at_gauges(pf_outputs, out_dir, start_date=None):
        flow_data = get_flow_at_gauges(pf_outputs, out_dir, start_date)
        if flow_data is not None:
            write_flows_to_csv(flow_data, out_dir)


def main():
    # parse the command line arguments
    args = parse_args(sys.argv[1:])
    # make csv files for each gauge site
    generate_flow_at_gauges(args.pf_outputs, args.out_dir, args.start_date)


if __name__ == '__main__':
    main()
