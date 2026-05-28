/**
 * Zigbee2MQTT External Converter for ESP32-H2 Dual Thermometer + Contact Sensor
 *
 * Location: Place in Zigbee2MQTT configuration directory under 'external_converters' folder
 * Configuration: Add to configuration.yaml:
 *   external_converters:
 *     - esp32h2_thermometer.js
 *
 * Device: ESP32-H2 with dual DS18B20 temperature sensors + contact sensor
 * Endpoints: 11 (temp sensor 1), 12 (temp sensor 2), 13 (contact/door sensor)
 */

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;

const definition = {
    zigbeeModel: ['ESP32H2.TH'],
    model: 'ESP32H2-TEMP-CONTACT',
    vendor: 'Espressif',
    description: 'ESP32-H2 Dual DS18B20 Temperature Sensor + Contact Sensor (Zigbee Router)',
    fromZigbee: [fz.temperature, fz.ias_contact_alarm_1],
    toZigbee: [],
    exposes: [
        e.temperature().withEndpoint('sensor1'),
        e.temperature().withEndpoint('sensor2'),
        e.contact().withEndpoint('contact'),
    ],
    endpoint: (device) => {
        return {
            'sensor1': 11,
            'sensor2': 12,
            'contact': 13,
        };
    },
    meta: {
        multiEndpoint: true,
    },
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint1 = device.getEndpoint(11);
        const endpoint2 = device.getEndpoint(12);
        const endpoint3 = device.getEndpoint(13);

        // Bind temperature measurement cluster for both temp endpoints
        await reporting.bind(endpoint1, coordinatorEndpoint, ['msTemperatureMeasurement']);
        await reporting.bind(endpoint2, coordinatorEndpoint, ['msTemperatureMeasurement']);

        // Bind IAS Zone cluster for contact sensor endpoint
        await reporting.bind(endpoint3, coordinatorEndpoint, ['ssIasZone']);

        // Configure temperature reporting
        // Min: 10 seconds, Max: 300 seconds (5 min), Change: 100 (1°C)
        await reporting.temperature(endpoint1, {min: 10, max: 300, change: 100});
        await reporting.temperature(endpoint2, {min: 10, max: 300, change: 100});

        // Enroll IAS Zone (Z2M handles CIE address write automatically)
        const endpoint3IasZone = endpoint3.getInputClusters().find(c => c.ID === 1280);
        if (endpoint3IasZone) {
            await endpoint3.read('ssIasZone', ['zoneState', 'zoneType', 'zoneStatus']);
        }

        logger.info('ESP32-H2 Dual Thermometer + Contact Sensor configured successfully');
    },
};

module.exports = definition;
