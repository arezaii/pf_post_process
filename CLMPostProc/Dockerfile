FROM hydroframe/fortranpython

COPY Scripts/ /testing/

WORKDIR /testing/

RUN python -m numpy.f2py -c primes.f95 pfb_read.f90 -m primes

CMD python checkme.py
