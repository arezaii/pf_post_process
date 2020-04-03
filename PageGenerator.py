import os
import argparse
import glob
import sys
from yattag import Doc, indent

DOCUMENT_ROOT='/var/www/subset/'


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
    parser = argparse.ArgumentParser('Generate a webpage to display hydrograph results')

    parser.add_argument("--png_dir", "-d", dest="png_dir", required=True,
                        help="path to the png files to display")
                        

    parser.add_argument("--out_file_name", "-n", dest="out_file", required=False,
                        help="path and name of the page to generate")

    parser.add_argument("--zip_file", "-z", dest="zip_file", required=True,
                        help="path to the tar.gz of the output to make available for download")

    return parser.parse_args(args)


def make_page(doc, tag, text, line, pngs, download_path):
    doc.asis('<!DOCTYPE html>')
    with tag('html'):
        with tag('body'):
            with tag('h1'):
                text('Job Finished!')

            with tag('div', id='download-container'):
                with tag('form', method='get', action=os.path.relpath(download_path,start=DOCUMENT_ROOT)):
                    with tag('button', type='submit'):
                        text('Download this data')
            
            if len(pngs) > 0:
                with tag('h2'):
                    text('Hydrographs')
                with tag('div', id='photo-container'):
                    # write each png
                    for png in pngs:
                        doc.stag('img', src=os.path.relpath(png,start=DOCUMENT_ROOT), klass="photo")
            else:
                with tag('p'):
                    text('This subset did not produce any hydrographs at known USGS locations.')
            with tag('div', id='new-subset-container'):
                with tag('a', href='http://subset.cuahsi.org/parflow/v1_0'):
                    text('Try another subset')
            with tag('div', id='download-parflow-container'):
                with tag('p'):
                    text('You can run this simulation on your own computer! Just download the data using the link above and use the Parflow Docker image.')
                with tag('a', href='https://github.com/parflow/docker'):
                    text('Download ParFlow Docker')

    return indent(doc.getvalue())


def get_rel_path(path):
    if path.startswith(DOCUMENT_ROOT):
        return path[len(DOCUMENT_ROOT):]


# get all the png files from the folder
def get_pngs(png_dir):
    return glob.glob(os.path.join(png_dir, '*.png'))


def main():
    args = parse_args(sys.argv[1:])
    pngs = get_pngs(args.png_dir)
    doc, tag, text, line = Doc().ttl()
    html = make_page(doc, tag, text, line, pngs, args.zip_file)
    if args.out_file is None:
        print(html)
    else:
        page = open(args.out_file, 'w')
        page.write(html)


if __name__ == '__main__':
    main()

