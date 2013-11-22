import utils

utils.initialize_build()
name = "dionysus"
out_path = utils.create_bin_name(name)

env = Environment(CPPPATH=[
                    '/usr/include/libusb-1.0',
                    '/usr/include/libftdi1'],
                  LIBPATH=['/usr/lib/',
                           '/usr/local/lib'],
                  LIBS=['ftdipp1',
                        'ftdi1'])

#Add the include file
src_files = ['src/main.cpp', 'src/dionysus.cpp']
#src_files = ['src/main.cpp', 'src/dionysus.cpp', 'src/nysa.cpp']
env.Program (out_path,
             src_files)


