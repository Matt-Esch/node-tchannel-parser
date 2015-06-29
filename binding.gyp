{
  "targets": [
    {
      "target_name": "tchannel_parser",
      "sources": [ "tchannel_parser.cc" ],
      "include_dirs": ["<!(node -e \"require('nan')\")"]
    }
  ]
}
