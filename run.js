
'use strict';

const addon = require('./build/Release/addon.node');

const checkBench = () =>{
    let sum = 0;
    for(let i = 0; i < 10000000000; ++i) {
        sum += i;
    }
    return sum;
}
(async () => {
    console.log(addon.hello());
    console.log(addon.hello1(1, 2));
    console.log('sync:', addon.hashPassword('password', 'hash343454523f', 20));
    const hash = await addon.hashPasswordAsync('password', 'hash343454523f', 20);
    addon.sleepThread(1000);
    console.log('async:', hash);
    console.log('sync:', addon.benchmarkSync(checkBench));
})();