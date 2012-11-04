# -*- mode: python -*-

# sfxr-dssi Waf build script

import sys

APPNAME='sfxr-dssi'
VERSION='20121103' # !FIX!

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')

def configure(conf):
    conf.load('compiler_c')
    conf.check_cfg(package='gtk+-2.0', atleast_version='2.4.0', mandatory = True)
    conf.check_cfg(package='libwhygui', uselib_store='WHYGUI', atleast_version='20121103',
                   mandatory = True, args = '--cflags --libs')
    conf.check_cfg(package='liblo', uselib_store='LO', atleast_version='0.23',
                   mandatory = True, args = '--cflags --libs')

    alsapkg = 'alsa'
    if sys.platform.startswith('darwin'): alsapkg = 'libdssialsacompat'
    conf.check_cfg(package=alsapkg, uselib_store='DSSI',
                   mandatory = True, args = '--cflags --libs')
    conf.check_cfg(package='dssi', uselib_store='DSSI', atleast_version='0.9',
                   mandatory = True, args = '--cflags --libs')

    conf.env.append_unique('CFLAGS', '-DPACKAGE_VERSION="%s"' % VERSION)
    conf.env.append_unique('CFLAGS', ['-Wall', '-Wunused'])

    have_O = False
    if 'CFLAGS' in conf.env:
        for k in conf.env['CFLAGS']:
            if k.startswith("-O") or k == '-g':
                have_O = True
                break
    if not have_O:
        conf.env.append_unique('CFLAGS', ['-O2', '-g'])

def build(bld):

    # DSSI plugin
    plugin_env = bld.env.copy()
    plugin_env.cshlib_PATTERN = '%s.so'
    plugin_env.macbundle_PATTERN = '%s.so'
    print('*******************', bld.env.CC_NAME, bld.env['CFLAGS'])
    if plugin_env.CC_NAME == 'gcc' and '-ffast-math' not in plugin_env['CFLAGS']:
        plugin_env.append_unique('CFLAGS', ['-finline-functions', '-ffast-math', '-fomit-frame-pointer', '-funroll-loops', '-Winline'])
        # plugin_env.append_unique('CCFLAGS', '-finline-limit=5000')
        # -FIX- this assumes gcc 3.4.6 or later:
        # plugin_env.append_unique('CCFLAGS', ['--param large-function-growth=NNN', '--param inline-unit-growth=NNN'])
    bld.shlib(source = ['src/sfxr.c', 'src/sfxr_ports.c', 'src/main.c'],
              target = 'sfxr-dssi',
              use = 'DSSI',
              install_path = '${PREFIX}/lib/dssi',
              env = plugin_env,
              mac_bundle = sys.platform.startswith('darwin'))

    # GUI executable
    bld.program(source = ['src/gui_main.c', 'src/gui_sfxr.c', 'src/sfxr_ports.c'],
                target = 'sfxr-dssi_gtk',
                use = 'LO WHYGUI',
                install_path = '${PREFIX}/lib/dssi/sfxr-dssi')

    # GUI Lua files
    bld.install_files('${PREFIX}/lib/dssi/sfxr-dssi', bld.path.ant_glob('src/*.lua'))

    # GUI images
    bld.install_files('${PREFIX}/lib/dssi/sfxr-dssi', bld.path.ant_glob('images/*'))

