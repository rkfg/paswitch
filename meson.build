project('paswitch', 'c',
  version : '1.0',
  default_options : ['warning_level=3'
                     ])

#ide:editable-filelist
sources = [
  'paswitch.c',
]

conf = configuration_data()
libnotify = dependency('libnotify', required : false)
if libnotify.found()
  conf.set('libnotify', true)
endif

configure_file(input: 'config.h.in', output: 'config.h', configuration : conf)

exe = executable('paswitch', sources, dependencies : [dependency('libpulse'), libnotify], install : true)
