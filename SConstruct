import utils

utils.initialize_build()
name = "dionysus-nysa-test"
libname = "nysa"
out_path = utils.create_bin_name(name)
out_lib = utils.create_bin_name(libname)

env = Environment(CFLAGS=[
                  '-fPIC'],
                  CPPFLAGS=[
                  '-fPIC'],
                  CPPPATH=[
                    '/usr/include/libusb-1.0',
                    '/usr/include/libftdi1',
                    'include'],
                  LIBPATH=['/usr/lib/',
                           '/usr/local/lib'],
                  LIBS=['ftdipp1',
                        'ftdi1'])





src_files = utils.get_source_list(base = "src", recursive = True)
test_files = ["./test/main.cpp"]
test_files.append(src_files)

env.Program (out_path,
             test_files)
static_lib = env.StaticLibrary(target = out_lib, source = src_files)


#VLC Plugin
vlc_video_plugin_name = "nysa_video_plugin"
out_vlc_plugin_path = utils.create_bin_name(vlc_video_plugin_name)
install_dir = "/usr/lib/vlc/plugins/video_output"
vlc_env = Environment(CPPPATH=[
                      '/usr/include/vlc',
                      '/usr/include/vlc/plugins',
                      'include',
                      '/usr/include/libusb-1.0',
                      '/usr/include/libftdi1'],
                    CFLAGS=[
                      '-std=gnu99',
                      '-g',
                      '-Wall',
                      '-Wextra',
                      '-O2',
                      '-DPIC',
                      '-fPIC'
                    ],
                    LIBPATH=[
                      '/usr/lib/',
                      '/usr/local/lib',
                      'lib'],
                    LIBS=[
                      'ftdipp1',
                      'libusb-1.0',
                      'libvlc',
                      'ftdi1'])
vlc_env.MergeFlags('!pkg-config --cflags vlc-plugin')
vlc_env.MergeFlags('!pkg-config --libs vlc-plugin')
vlc_env.MergeFlags('-Wl,-no-undefined,-z,defs,-fPIC')

vlc_files = ["./test/nysa_video.cpp"]
vlc_files.append(src_files)

vlc_env.Alias('install', [install_dir])


sl = vlc_env.SharedLibrary(target = out_vlc_plugin_path, source = vlc_files)
vlc_env.Install(dir = install_dir, source = sl)


