# Makefile.am for VSFS's Python API

VOBLA_LIBS = ${top_srcdir}/lib/vobla/vobla/.libs
CLEANFILES = *.o
# Workaround
CXXFLAGS = -std=c++11 -g -I${top_srcdir}/lib/vobla -I${top_srcdir} \
	   $(PYTHON_CPPFLAGS)
LDFLAGS = $(PYTHON_LDFLAGS) $(BOOST_PYTHON_LIB)

vsfs_py.o: vsfs_py.cpp
	$(CXX) $(AM_CXXFLAGS) ${CXXFLAGS} -fPIC -fpic -c -o $@ $<

vsfs_py.so: vsfs_py.o
	cp $(VOBLA_LIBS)/*.so .
	$(CXX) -fPIC -fpic $< libvobla.so -shared -lboost_python $(AM_LDFLAGS) -o $@

all: vsfs_py.so
