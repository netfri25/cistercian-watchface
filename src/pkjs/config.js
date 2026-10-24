module.exports = [
    {
        "type": "heading",
        "defaultValue": "Watchface Settings"
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Colors"
            },
            {
                "type": "color",
                "messageKey": "StrokeColor",
                "defaultValue": "0x000000",
                "label": "Stroke Color"
            },
            {
                "type": "color",
                "messageKey": "BackgroundColor",
                "defaultValue": "0xFFFFFF",
                "label": "Background Color"
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Preferences"
            },
            {
                "type": "toggle",
                "messageKey": "AntiAliasing",
                "defaultValue": true,
                "label": "Enable Antialiasing"
            },
            {
                "type": "toggle",
                "messageKey": "ShowDate",
                "defaultValue": true,
                "label": "Show Date"
            },
            {
                "type": "input",
                "messageKey": "Padding",
                "defaultValue": 25,
                "label": "Padding",
                "serializeValueAs": "integer",
                "attributes": {
                    "type": "number"
                }
            },
            {
                "type": "input",
                "messageKey": "TimeStrokeWidth",
                "defaultValue": 7,
                "label": "Time Stroke Width",
                "serializeValueAs": "integer",
                "attributes": {
                    "type": "number"
                }
            },
            {
                "type": "input",
                "messageKey": "DateStrokeWidth",
                "defaultValue": 7,
                "label": "Date Stroke Width",
                "serializeValueAs": "integer",
                "attributes": {
                    "type": "number"
                }
            },
            {
                "type": "input",
                "messageKey": "DateScalePercent",
                "defaultValue": 28,
                "label": "Date Scale Percent",
                "serializeValueAs": "integer",
                "attributes": {
                    "type": "number"
                }
            }
        ]
    },
    {
        "type": "submit",
        "defaultValue": "Save Settings"
    }
];
