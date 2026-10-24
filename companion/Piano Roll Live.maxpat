{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 0,
      "revision": 0,
      "architecture": "x64",
      "modernui": 1
    },
    "classnamespace": "box",
    "rect": [
      100,
      100,
      800,
      480
    ],
    "openinpresentation": 1,
    "default_fontsize": 12.0,
    "devicewidth": 580.0,
    "boxes": [
      {
        "box": {
          "id": "midi-in",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            260,
            60,
            22
          ],
          "text": "midiin"
        }
      },
      {
        "box": {
          "id": "midi-out",
          "maxclass": "newobj",
          "patching_rect": [
            20,
            310,
            60,
            22
          ],
          "text": "midiout"
        }
      },
      {
        "box": {
          "id": "ready",
          "maxclass": "newobj",
          "patching_rect": [
            130,
            260,
            100,
            22
          ],
          "text": "live.thisdevice"
        }
      },
      {
        "box": {
          "id": "init",
          "maxclass": "message",
          "patching_rect": [
            130,
            290,
            40,
            22
          ],
          "text": "init"
        }
      },
      {
        "box": {
          "id": "defer",
          "maxclass": "newobj",
          "patching_rect": [
            200,
            330,
            70,
            22
          ],
          "text": "deferlow"
        }
      },
      {
        "box": {
          "id": "api",
          "maxclass": "newobj",
          "patching_rect": [
            200,
            370,
            130,
            22
          ],
          "text": "js bridge-max.js"
        }
      },
      {
        "box": {
          "id": "node",
          "maxclass": "newobj",
          "patching_rect": [
            410,
            370,
            360,
            22
          ],
          "text": "node.script bridge-node.js @autostart 1 @defer 1"
        }
      },
      {
        "box": {
          "id": "metro",
          "maxclass": "newobj",
          "patching_rect": [
            260,
            260,
            90,
            22
          ],
          "text": "qmetro 300"
        }
      },
      {
        "box": {
          "id": "on",
          "maxclass": "message",
          "patching_rect": [
            260,
            230,
            30,
            22
          ],
          "text": "1"
        }
      },
      {
        "box": {
          "id": "poll",
          "maxclass": "message",
          "patching_rect": [
            260,
            300,
            40,
            22
          ],
          "text": "poll"
        }
      },
      {
        "box": {
          "id": "title",
          "maxclass": "comment",
          "patching_rect": [
            20,
            10,
            220,
            24
          ],
          "fontsize": 16.0,
          "text": "PIANO ROLL LIVE",
          "presentation": 1,
          "presentation_rect": [
            10,
            8,
            350,
            24
          ]
        }
      },
      {
        "box": {
          "id": "open",
          "maxclass": "textbutton",
          "patching_rect": [
            20,
            60,
            110,
            26
          ],
          "text": "Open editor",
          "presentation": 1,
          "presentation_rect": [
            10,
            38,
            110,
            26
          ]
        }
      },
      {
        "box": {
          "id": "open-msg",
          "maxclass": "message",
          "patching_rect": [
            20,
            100,
            90,
            22
          ],
          "text": "openeditor"
        }
      },
      {
        "box": {
          "id": "load",
          "maxclass": "textbutton",
          "patching_rect": [
            140,
            60,
            150,
            26
          ],
          "text": "Load selected clip",
          "presentation": 1,
          "presentation_rect": [
            128,
            38,
            150,
            26
          ]
        }
      },
      {
        "box": {
          "id": "load-msg",
          "maxclass": "message",
          "patching_rect": [
            140,
            100,
            70,
            22
          ],
          "text": "loadclip"
        }
      },
      {
        "box": {
          "id": "status",
          "maxclass": "comment",
          "patching_rect": [
            20,
            150,
            600,
            46
          ],
          "fontsize": 12.0,
          "text": "Select a MIDI clip, then Open editor.",
          "presentation": 1,
          "presentation_rect": [
            10,
            70,
            560,
            46
          ]
        }
      },
      {
        "box": {
          "id": "hint",
          "maxclass": "comment",
          "patching_rect": [
            20,
            200,
            610,
            30
          ],
          "fontsize": 11.0,
          "text": "Live plays the clip through its existing instrument. Apply sends edits back.",
          "presentation": 1,
          "presentation_rect": [
            10,
            118,
            560,
            28
          ]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "midi-in",
            0
          ],
          "destination": [
            "midi-out",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "ready",
            0
          ],
          "destination": [
            "init",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "init",
            0
          ],
          "destination": [
            "defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "defer",
            0
          ],
          "destination": [
            "api",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "api",
            0
          ],
          "destination": [
            "node",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "node",
            0
          ],
          "destination": [
            "defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "ready",
            0
          ],
          "destination": [
            "on",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "on",
            0
          ],
          "destination": [
            "metro",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "metro",
            0
          ],
          "destination": [
            "poll",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "poll",
            0
          ],
          "destination": [
            "defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "open",
            0
          ],
          "destination": [
            "open-msg",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "open-msg",
            0
          ],
          "destination": [
            "defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "load",
            0
          ],
          "destination": [
            "load-msg",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "load-msg",
            0
          ],
          "destination": [
            "defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "api",
            1
          ],
          "destination": [
            "status",
            0
          ]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "bridge-max.js",
        "type": "TEXT",
        "implicit": 1
      },
      {
        "name": "bridge-node.js",
        "type": "TEXT",
        "implicit": 1
      },
      {
        "name": "clip-protocol.js",
        "type": "TEXT",
        "implicit": 1
      }
    ]
  }
}
