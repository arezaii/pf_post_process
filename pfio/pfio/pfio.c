/* 
* a test program to read in PFB
*/
#include "Python.h"
#include "endian.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"

/*#include "readpfb.h"*/

/* #### Utility functions ######################### */
typedef unsigned char byte;

int BigEndianSystem;

short (*BigShort) ( short s );
short (*LittleShort) ( short s );
int (*BigLong) ( int i );
int (*LittleLong) ( int i );
double (*BigFloat) ( double f );
double (*LittleFloat) ( double f );


//adapted from Quake 2 source and tools_io.c

void tools_WriteFloat(
                      FILE * file,
                      float *ptr,
                      int    len)
{
  int i;
  float *data;

  union {
//      double number;
//      char buf[8];
    float number;
    char buf[4];
  } a, b;

  /* write out each double with bytes swaped                               */
  for (i = len, data = ptr; i--; )
  {
    a.number = *data++;
    b.buf[0] = a.buf[3];
    b.buf[1] = a.buf[2];
    b.buf[2] = a.buf[1];
    b.buf[3] = a.buf[0];

    fwrite(&b.number, sizeof(float), 1, (FILE*)file);
  }
}

void tools_WriteDouble(
                       FILE *  file,
                       double *ptr,
                       int     len)
{
  int i;
  double *data;

  union {
    double number;
    char buf[8];
  } a, b;

  /* write out each double with bytes swaped                               */
  for (i = len, data = ptr; i--; )
  {
    a.number = *data++;
    b.buf[0] = a.buf[7];
    b.buf[1] = a.buf[6];
    b.buf[2] = a.buf[5];
    b.buf[3] = a.buf[4];
    b.buf[4] = a.buf[3];
    b.buf[5] = a.buf[2];
    b.buf[6] = a.buf[1];
    b.buf[7] = a.buf[0];

    fwrite(&b.number, sizeof(double), 1, (FILE*)file);
  }
}

void tools_WriteInt(
                    FILE * file,
                    int *  ptr,
                    int    len)
{
  int i;
  int *data;

  union {
    long number;
    char buf[4];
  } a, b;


  /* write out int with bytes swaped                                       */
  for (i = len, data = ptr; i--; )
  {
    a.number = *data++;
    b.buf[0] = a.buf[3];
    b.buf[1] = a.buf[2];
    b.buf[2] = a.buf[1];
    b.buf[3] = a.buf[0];

    fwrite(&b.number, sizeof(int), 1, (FILE*)file);
  }
}

short ShortSwap( short s )
{
	byte b1, b2;
	
	b1 = s & 255;
	b2 = (s >> 8) & 255;

	return (b1 << 8) + b2;
}

short ShortNoSwap( short s )
{
	return s;
}

int LongSwap (int i)
{
	byte b1, b2, b3, b4;

	b1 = i & 255;
	b2 = ( i >> 8 ) & 255;
	b3 = ( i>>16 ) & 255;
	b4 = ( i>>24 ) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

int LongNoSwap( int i )
{
	return i;
}

double FloatSwap( double f )
{
	union
	{
	    double f;
	    char b[8];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[7];
	dat2.b[1] = dat1.b[6];
	dat2.b[2] = dat1.b[5];
	dat2.b[3] = dat1.b[4];
	dat2.b[4] = dat1.b[3];
	dat2.b[5] = dat1.b[2];
	dat2.b[6] = dat1.b[1];
	dat2.b[7] = dat1.b[0];	
	
	return dat2.f;
}

double FloatNoSwap( double f )
{
	return f;
}

void InitEndian( void )
{
	//clever little trick from Quake 2 to determine the endian
	//of the current system without depending on a preprocessor define

	byte SwapTest[2] = { 1, 0 };
	
	if( *(short *) SwapTest == 1 )
	{
		//little endian
		BigEndianSystem = 1;
               // printf("This is a little endian machine!\n");
		//set func pointers to correct funcs
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		//big endian
		BigEndianSystem = 0;
                //printf("This is a big endian machine!\n");

		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
}

/* #### Main read and write pfb files functions ######################### */

static PyObject * pfread(PyObject *self, PyObject *args);

static PyObject * pfwrite(PyObject *self, PyObject *args);

static PyMethodDef pf_funcs[] = { { "pfread", pfread, METH_VARARGS },
									{ "pfwrite", pfwrite, METH_VARARGS },
                                          { NULL, NULL } };
   
/* ReadPFB(char *file_name)*/
static PyObject * pfread(PyObject *self, PyObject *args)
{
    FILE *fp;
    
    char *pfbfile;
    
    if (!PyArg_ParseTuple(args, "s", &pfbfile)) {
      return NULL;
   }
    
    //double * data2 = (double *) data1;  
    //double * outdata2 = (double *) outdata;
   
    int        counter;
    //struct  hdr1 my_hdr1;
    /*struct  hdr2 my_hdr2;*/
    
    PyArrayObject *array_out;
    
    npy_intp dims[3];
    
    double *val_array;
    
    double     X, Y, Z;
    int        NX, NY, NZ, N;
    //int        NX, NY, NZ;
    double     DX, DY, DZ;
    int        num_subgrids;
    
    int        x, y, z;
    int        nx, ny, nz;
    int        rx, ry, rz;
    
    int        nsg, j, k, i;
    
    int        qq;
    
    //double     *ptr;
        
    double     val;
   //double     *data1;
    
    //unsigned char bytes[8];
    //int sum = 0;
    //FILE *fp=fopen("Test.pfb","rb");
    //while ( fread(bytes, 4, 1,fp) != 0) {
    //    sum += bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24);
    //}
    
    /* open the input file */
    if ((fp = fopen(pfbfile, "rb")) == NULL) {
      //return 2.; //NULL;
      printf("uh oh - can't open the file...\n");
      return NULL;
      }
    //fp = fopen("Test.pfb", "rb");
    /*if (!fp)
		{
			printf("Unable to open file!");
			return 1;
		} */
         
    /* read in header information */
    InitEndian();
    //printf("BigEndian = %i\n", BigEndianSystem);
    /* read in header info - as taken from readdatabox.c*/
    fread(&X, 8, 1, fp);
    fread(&Y, 8, 1, fp);
    fread(&Z, 8, 1, fp);
	
    X = LittleFloat(X);
    Y = LittleFloat(Y);
    Z = LittleFloat(Z);
    
    fread(&NX, 4, 1, fp);
    fread(&NY, 4, 1, fp);
    fread(&NZ, 4, 1, fp);
        
    NX = BigLong(NX);
    NY = BigLong(NY);
    NZ = BigLong(NZ);
    
    dims[0] = NZ;
    dims[1] = NY;
    dims[2] = NX;
    N = NZ*NY*NX;
    
    array_out = (PyArrayObject*) PyArray_SimpleNew(3, dims, NPY_DOUBLE);
    val_array = (double*)PyArray_DATA(array_out);
    
    //val_array = (double *)malloc(sizeof(double) * N);
    //double val_array[NZ][NY][NX];
    
    fread(&DX, 8, 1, fp);
    fread(&DY, 8, 1, fp);
    fread(&DZ, 8, 1, fp);
    
    DX = BigFloat(DX);
    DY = BigFloat(DY);
    DZ = BigFloat(DZ);
    
    fread(&num_subgrids, 4, 1, fp);
    num_subgrids = BigLong(num_subgrids);
     
    //printf("X = %f \n Y = %f \n Z = %f \n", X, Y, Z);
    //printf("NX = %i \n NY = %i \n NZ = %i \n", NX, NY, NZ);
    //printf("DX = %f \n DY = %f \n DZ = %f \n Subgrids = %i \n", DX, DY, DZ, num_subgrids);
    
    counter = 0;
    /* read in the databox data - from readdatabox.c - HOW TO PUT DATA INTO ARRAY to PASS TO PYTHON???*/
    for (nsg = num_subgrids; nsg--;)
    {
      fread(&x, 4, 1, fp);
      fread(&y, 4, 1, fp);
      fread(&z, 4, 1, fp);
      
       x = BigLong(x);
       y = BigLong(y);
       z = BigLong(z);
      
      fread(&nx, 4, 1, fp);
      fread(&ny, 4, 1, fp);
      fread(&nz, 4, 1, fp);
      
       nx = BigLong(nx);
       ny = BigLong(ny);
       nz = BigLong(nz);

      fread(&rx, 4, 1, fp);
      fread(&ry, 4, 1, fp);
      fread(&rz, 4, 1, fp);
      //printf("nx = %i,  x = %i \n ny = %i,  y = %i \nnz = %i,  z = %i \n", nx, x,ny, y,nz, z);
      for (k = 0; k < nz; k++)
      {
          //for (j = ny-1; j >=0; j--) //flip along y axis
          for (j = 0; j <ny; j++)
          {
              for (i = 0; i < nx; i++)
	          {   
	              //qq = (x+i)*(y+j)*(z+k);
	              qq = ((z+k)*(NX*NY)) + ((NX)*(y+j)+(x+i));
	              fread(&val, 8, 1, fp);
	              val = BigFloat(val);
	              val_array[qq] = (double)val;
	              //outdata2[qq] = data2[qq] * 2;
                      //counter += 1;
	              //val_array[(x+i)][(y+j)][(z+k)] = val;
	              //ptr = DataboxCoeff(v, x, (y + j), (z + k));
	              //tools_ReadDouble(fp, ptr,  nx);
	           }
	       }
	    }
	}
    fclose(fp);
    //return 0;
    //Trying to reverse the array
    for (k = 0; k < NZ; k++)
    {
    	for (j = 0; j < NY/2; j++)
    	{
    		for (i = 0; i < NX; i++)
    		{
    			double temp = val_array[k*NX*NY+j*NX+i];
				val_array[k*NX*NY+j*NX+i]=val_array[k*NX*NY+(NY-j-1)*NX+i];
				val_array[k*NX*NY+(NY-j-1)*NX+i] = temp; 
    		}
    	}
    }
    return PyArray_Return(array_out);
    //return data1;
}

static PyObject * pfwrite(PyObject *self, PyObject *args)
{
    FILE *fp;
    
    PyArrayObject *matin;
    
    char *pfbfile;
    
    double *val_array;
    double *new_val_array;
    PyArrayObject *array_out;
    
    int        j, k, i, iteri;
    int 		num_iter;
    npy_intp dims[3];
	
    double     X, Y, Z;
    int        NX, NY, NZ;
    double     DX, DY, DZ;
    
    if (!PyArg_ParseTuple(args, "O!sdddddd", &PyArray_Type, &matin, &pfbfile,
    										&X, &Y, &Z,
    										&DX, &DY, &DZ)) {
      return NULL;
   }
    
    NZ = PyArray_DIMS(matin)[0];
    NY = PyArray_DIMS(matin)[1];
    NX = PyArray_DIMS(matin)[2];
    
	val_array = (double*)PyArray_DATA(matin);
    
    fp = fopen(pfbfile, "w");
    
    int ns = 1;
    int x = 0;
	int y = 0;
	int z = 0;
	int nx = NX;
	int ny = NY;
	int nz = NZ;
	int rx = 1;
	int ry = 1;
	int rz = 1;
    
    tools_WriteDouble(fp, &X, 1);
	tools_WriteDouble(fp, &Y, 1);
	tools_WriteDouble(fp, &Z, 1);
	tools_WriteInt(fp, &NX, 1);
	tools_WriteInt(fp, &NY, 1);
	tools_WriteInt(fp, &NZ, 1);
	tools_WriteDouble(fp, &DX, 1);
	tools_WriteDouble(fp, &DY, 1);
	tools_WriteDouble(fp, &DZ, 1);
	
	tools_WriteInt(fp, &ns, 1);

	tools_WriteInt(fp, &x, 1);
	tools_WriteInt(fp, &y, 1);
	tools_WriteInt(fp, &z, 1);
	tools_WriteInt(fp, &nx, 1);
	tools_WriteInt(fp, &ny, 1);
	tools_WriteInt(fp, &nz, 1);
	tools_WriteInt(fp, &rx, 1);
	tools_WriteInt(fp, &ry, 1);
	tools_WriteInt(fp, &rz, 1);
	
	//Trying to reverse back the array
	
	if (NZ < 3) {
		dims[0] = NZ;
		dims[1] = NY;
		dims[2] = NX;
	
		array_out = (PyArrayObject*) PyArray_SimpleNew(3, dims, NPY_DOUBLE);
		new_val_array = (double*)PyArray_DATA(array_out);
		for (k = 0; k < NZ; k++)
		{
			for (j = 0; j < (NY/2 +1) ; j++)
			{
				for (i = 0; i < NX; i++)
				{
					//double temp = val_array[k*NX*NY+j*NX+i];
					//val_array[k*NX*NY+j*NX+i]=val_array[k*NX*NY+(NY-j-1)*NX+i];
					//val_array[k*NX*NY+(NY-j-1)*NX+i] = temp; 
					new_val_array[k*NX*NY+j*NX+i]=val_array[k*NX*NY+(NY-j-1)*NX+i];
					new_val_array[k*NX*NY+(NY-j-1)*NX+i]=val_array[k*NX*NY+j*NX+i];
				}
			}
		}
	
		tools_WriteDouble(fp, new_val_array, nx*ny*nz);
	}
	else {
		num_iter = NZ/3 +1 ;
		//printf("num_iter = %i \n", num_iter);
		for (iteri = 0; iteri < num_iter; iteri++) {
			int starti = iteri*3;
			int endi = (iteri+1)*3;
			int mini_nz = 3;
			if (endi > NZ) {
				endi = NZ;
				mini_nz = endi - starti;
			}
			//printf("starti = %i \n endi = %i \n mini_nz = %i \n", starti, endi, mini_nz);
			dims[0] = mini_nz;
			dims[1] = NY;
			dims[2] = NX;
	
			array_out = (PyArrayObject*) PyArray_SimpleNew(3, dims, NPY_DOUBLE);
			new_val_array = (double*)PyArray_DATA(array_out);
			for (k = starti; k < endi; k++)
			{
				for (j = 0; j < (NY/2 +1) ; j++)
				{
					for (i = 0; i < NX; i++)
					{
						//double temp = val_array[k*NX*NY+j*NX+i];
						//val_array[k*NX*NY+j*NX+i]=val_array[k*NX*NY+(NY-j-1)*NX+i];
						//val_array[k*NX*NY+(NY-j-1)*NX+i] = temp; 
						new_val_array[(k-iteri*3)*NX*NY+j*NX+i]=val_array[k*NX*NY+(NY-j-1)*NX+i];
						new_val_array[(k-iteri*3)*NX*NY+(NY-j-1)*NX+i]=val_array[k*NX*NY+j*NX+i];
					}
				}
			}
	
			tools_WriteDouble(fp, new_val_array, nx*ny*mini_nz);
		}
	}
	
	Py_RETURN_NONE;
}

void freeme(double *aptr)
{
    printf("freeing address: %p\n", aptr);
    free(aptr);
}


static struct PyModuleDef pfio =
{
    PyModuleDef_HEAD_INIT,
    "pfio", /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    pf_funcs
};

PyMODINIT_FUNC PyInit_pfio(void)
{
    PyObject* po = PyModule_Create(&pfio);
  	import_array();
    return po;
}




