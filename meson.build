project('paswitch', 'c',
  version : '1.0',
  default_options : ['warning_level=3'
                     ])

#ide:editable-filelist
sources = [
  'paswitch.c',
]

exe = executable('paswitch', sources, dependencies : dependency('libpulse'), install : true)
