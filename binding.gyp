{
    "targets": [
        {
            "target_name": "hcifunctions",
            "sources": [
                "hci_updown.cpp",
                "hci_list.cpp",
                "hci_spoof_mac.cpp",
                "hci.cpp"
            ],
            "cflags": [ "-fpermissive" ],
            "link_settings": {
                "libraries": [
                    "-lbluetooth",
                ],
            },
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ]
        },
    ]
}
