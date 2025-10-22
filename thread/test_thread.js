'use strict';

// Load the thread worker addon
const threadWorker = require('../build/Release/addon.node');

console.log('=== Simple Thread Worker Demo ===\n');

// Start the background worker
console.log('Starting background worker...');
threadWorker.startWorker((message, count) => {
    console.log(`[${new Date().toISOString()}] ${message}: ${count}`);
});

// Check if worker is running
console.log('Worker running:', threadWorker.isWorkerRunning());

// Let it run for 5 seconds
setTimeout(() => {
    console.log('\nStopping worker...');
    threadWorker.stopWorker();
    console.log('Worker running:', threadWorker.isWorkerRunning());
    console.log('\nDemo completed!');
}, 5000);
