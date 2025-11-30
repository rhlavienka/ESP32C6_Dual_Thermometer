/**
 * Zigbee2MQTT External Converter pre ESP32-C6 Dual Thermometer
 * 
 * Umiestnenie: Do adresára s Zigbee2MQTT konfiguráciou
 * Konfigurácia: Pridať do configuration.yaml:
 *   external_converters:
 *     - esp32c6_thermometer.js
 */

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;

const definition = {
    zigbeeModel: ['ESP32C6.TH'],
    model: 'ESP32C6-DUAL-TEMP',
    vendor: 'Espressif',
    description: 'ESP32-C6 Dual DS18B20 Temperature Sensor with Zigbee Router',
    fromZigbee: [fz.temperature],
    toZigbee: [],
    exposes: [
        e.temperature().withEndpoint('sensor1'),
        e.temperature().withEndpoint('sensor2'),
    ],
    endpoint: (device) => {
        return {
            'sensor1': 11,
            'sensor2': 12,
        };
    },
    meta: {
        multiEndpoint: true,
    },
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint1 = device.getEndpoint(11);
        const endpoint2 = device.getEndpoint(12);
        
        // Bind temperature measurement cluster for both endpoints
        await reporting.bind(endpoint1, coordinatorEndpoint, ['msTemperatureMeasurement']);
        await reporting.bind(endpoint2, coordinatorEndpoint, ['msTemperatureMeasurement']);
        
        // Configure temperature reporting
        // Min: 10 seconds, Max: 300 seconds (5 min), Change: 100 (1°C)
        await reporting.temperature(endpoint1, {min: 10, max: 300, change: 100});
        await reporting.temperature(endpoint2, {min: 10, max: 300, change: 100});
        
        logger.info('ESP32C6 Dual Thermometer configured successfully');
    },
};

module.exports = definition;
