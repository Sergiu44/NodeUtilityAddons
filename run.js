
'use strict';

const addon = require('./build/Release/addon.node');

(async () => {
    console.log(addon.hello());
    console.log(addon.hello1(1, 2));
    console.log('sync:', addon.hashPassword('password', 'hash343454523f', 20));
    const hash = await addon.hashPasswordAsync('password', 'hash343454523f', 20);
    console.log('async:', hash);
})();