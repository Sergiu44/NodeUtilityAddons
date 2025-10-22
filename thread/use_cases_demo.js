'use strict';

const addon = require('../build/Release/addon.node');

console.log('=== Thread Worker Use Cases Demo ===\n');

// Example 1: Progress Tracking
console.log('1. Progress Tracking Example:');
addon.startWorker((message, progress) => {
    console.log(`Progress: ${progress}% - ${message}`);
});

setTimeout(() => {
    addon.stopWorker();
    console.log('Progress tracking stopped.\n');
    
    // Example 2: Background Monitoring
    console.log('2. Background Monitoring Example:');
    addon.startWorker((message, data) => {
        const timestamp = new Date().toISOString();
        console.log(`[${timestamp}] System Status: ${message} - Value: ${data}`);
    });
    
    setTimeout(() => {
        addon.stopWorker();
        console.log('Monitoring stopped.\n');
        
        // Example 3: Data Processing Pipeline
        console.log('3. Data Processing Pipeline Example:');
        addon.startWorker((stage, count) => {
            console.log(`Processing Stage: ${stage} - Items processed: ${count}`);
        });
        
        setTimeout(() => {
            addon.stopWorker();
            console.log('Data processing completed!');
        }, 3000);
        
    }, 3000);
    
}, 3000);
