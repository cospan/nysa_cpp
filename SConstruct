import utils

utils.initialize_build()
name = "dionysus"
out_path = utils.create_bin_name(name)

env = Environment(CPPPATH=[
                    '/usr/include/libusb-1.0',
                    '/usr/include/libftdi1',
                    'include'],
                  LIBPATH=['/usr/lib/',
                           '/usr/local/lib'],
                  LIBS=['ftdipp1',
                        'ftdi1'])

src_files = utils.get_source_list(base = "src", recursive = True)
env.Program (out_path,
             src_files)


