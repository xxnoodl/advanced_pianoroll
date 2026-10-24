'use strict';
// Max imports this entry point; it is not Node's main module.
const maxApi = require('max-api');
try { require('./bridge-transport').start(maxApi); }
catch (error) {
    maxApi.post('Piano Roll launcher: ' + error.stack);
    maxApi.outlet('nodeerror', 'Launcher failed: ' + error.message);
}
