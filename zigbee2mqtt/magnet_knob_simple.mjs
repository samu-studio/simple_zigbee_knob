// Zigbee2MQTT external converter for the Simple Zigbee Knob
//
// Installation: copy to <z2m-data>/external_converters/, restart Z2M, pair the device and
// press "Reconfigure" once (sets up reporting; without it the firmware reports nothing).
//
// Result:
//   value_1, value_2, value_3 – values 1–3, 0–100 % (EP1–EP3). Follow the knob and can be
//                               set from Home Assistant.
//   active                    – selected value 1–3 (EP4), long press button 1/2/3 on the knob or set from HA
//   action                    – button click/double click (EP6): 1_click, 1_double_click, 2_…, 3_…
//                               (for automations: event entity or device trigger)
//   ring_brightness           – ring brightness 0–100 % (EP5), stored in the knob
//
// Writes go to genAnalogOutput, reports come from genAnalogInput of the same endpoint
// (with the ESP Zigbee SDK only Analog Input reports work reliably).

import * as exposes from 'zigbee-herdsman-converters/lib/exposes';

const e = exposes.presets;
const ea = exposes.access;

// Set up reporting (without the entry the firmware reports nothing). Up to three attempts, because
// right after the interview a request occasionally gets lost; a final failure should still not
// abort "Reconfigure". The firmware reports every change itself (throttled); the large threshold
// keeps the stack from reporting on its own (double radio traffic).
// Events (EP6) without periodic reports, otherwise the last event would come back.
const tryReporting = async (ep, maxInterval = 3600) => {
    for (let attempt = 0; attempt < 3; attempt++) {
        try {
            await ep.configureReporting('genAnalogInput', [
                {attribute: 'presentValue', minimumReportInterval: 0, maximumReportInterval: maxInterval, reportableChange: 1000},
            ]);
            return;
        } catch (err) {
            await new Promise((r) => setTimeout(r, 1000));
        }
    }
};

// One Analog In/Out endpoint as a number in HA
const analogNumber = ({key, ep, label, min, max, step, unit, description, category}) => {
    let expose = e.numeric(key, ea.ALL).withLabel(label).withValueMin(min).withValueMax(max)
        .withValueStep(step).withDescription(description);
    if (unit) expose = expose.withUnit(unit);
    if (category) expose = expose.withCategory(category);
    return {
        isModernExtend: true,
        exposes: [expose],
        fromZigbee: [
            {
                cluster: 'genAnalogInput',
                type: ['attributeReport', 'readResponse'],
                convert: (model, msg, publish, options, meta) => {
                    if (msg.endpoint.ID !== ep || msg.data.presentValue === undefined) return;
                    return {[key]: Math.round(msg.data.presentValue)};
                },
            },
        ],
        toZigbee: [
            {
                key: [key],
                convertSet: async (entity, k, value, meta) => {
                    const v = Math.max(min, Math.min(max, Math.round(Number(value))));
                    await meta.device.getEndpoint(ep).write('genAnalogOutput', {presentValue: v});
                    return {state: {[key]: v}};
                },
                convertGet: async (entity, k, meta) => {
                    await meta.device.getEndpoint(ep).read('genAnalogInput', ['presentValue']);
                },
            },
        ],
        configure: [
            async (device, coordinatorEndpoint) => {
                const endpoint = device.getEndpoint(ep);
                await endpoint.bind('genAnalogInput', coordinatorEndpoint);
                await tryReporting(endpoint);
                await endpoint.read('genAnalogInput', ['presentValue']);
            },
        ],
    };
};

const knobValue = (n) => analogNumber({
    key: `value_${n}`, ep: n, label: `Value ${n}`, min: 0, max: 100, step: 1, unit: '%',
    description: `Value ${n} (long press button ${n} selects it on the knob)`,
});

// Button events (EP6, code n*10 + 1 click / 2 double click) -> action
const ACTIONS = ['1_click', '1_double_click', '2_click', '2_double_click', '3_click', '3_double_click'];
const KIND = {1: 'click', 2: 'double_click'};
const knobEvents = () => ({
    isModernExtend: true,
    exposes: [e.action(ACTIONS)],
    fromZigbee: [
        {
            cluster: 'genAnalogInput',
            type: ['attributeReport'],  // reports only: a read would repeat the last event
            convert: (model, msg, publish, options, meta) => {
                if (msg.endpoint.ID !== 6 || msg.data.presentValue === undefined) return;
                const code = Math.round(msg.data.presentValue);
                const btn = Math.floor(code / 10), kind = KIND[code % 10];
                if (btn >= 1 && btn <= 3 && kind) return {action: `${btn}_${kind}`};
            },
        },
    ],
    configure: [
        async (device, coordinatorEndpoint) => {
            const ep = device.getEndpoint(6);
            await ep.bind('genAnalogInput', coordinatorEndpoint);
            await tryReporting(ep, 0xffff);
        },
    ],
});

// Read minPresentValue/maxPresentValue/resolution (set by the firmware on every analog endpoint)
// into the Z2M cache. If they are missing there, Z2M queries them on startup and hangs while the
// knob is unreachable. Press "Reconfigure" once after every new pairing.
const readAnalogMeta = (eps) => ({
    isModernExtend: true,
    configure: [
        async (device, coordinatorEndpoint) => {
            for (const [id, clusters] of eps) {
                const ep = device.getEndpoint(id);
                for (const cluster of clusters) {
                    try {
                        await ep.read(cluster, ['minPresentValue', 'maxPresentValue', 'resolution']);
                    } catch (err) {
                        // don't abort "Reconfigure"
                    }
                }
            }
        },
    ],
});

// Read the firmware version (genBasic.swBuildId)
const knobFirmware = () => ({
    isModernExtend: true,
    configure: [
        async (device, coordinatorEndpoint) => {
            await device.getEndpoint(1).read('genBasic', ['swBuildId']);
        },
    ],
});

export default {
    zigbeeModel: ['MagnetKnobSimple'],
    model: 'MagnetKnobSimple',
    vendor: 'DIY',
    description: 'Magnetic rotary knob: three values 0–100 %',
    extend: [
        knobEvents(),
        knobValue(1),
        knobValue(2),
        knobValue(3),
        analogNumber({
            key: 'active', ep: 4, label: 'Active value', min: 1, max: 3, step: 1,
            description: 'Which value is currently changed by rotating',
        }),
        analogNumber({
            key: 'ring_brightness', ep: 5, label: 'Ring brightness', min: 0, max: 100, step: 5, unit: '%',
            category: 'config', description: 'Brightness of the LED ring (0 = off)',
        }),
        knobFirmware(),
        readAnalogMeta([[1, ['genAnalogInput', 'genAnalogOutput']], [2, ['genAnalogInput', 'genAnalogOutput']],
            [3, ['genAnalogInput', 'genAnalogOutput']], [4, ['genAnalogInput', 'genAnalogOutput']],
            [5, ['genAnalogInput', 'genAnalogOutput']], [6, ['genAnalogInput']]]),
    ],
    meta: {},
};
