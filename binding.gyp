{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "napi_playground.cc", "thread/thread_worker.cc" ],
      "headers": [ "thread/thread_worker.h" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "OTHER_CFLAGS": [
          "-I/opt/homebrew/include"
        ],
        "OTHER_LDFLAGS": [
          "-L/opt/homebrew/lib",
          "-largon2"
        ]
      }
    }
  ]
}