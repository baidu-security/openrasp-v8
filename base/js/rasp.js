/**
 * @file
 */
'use strict';

global.RASP = class {
    constructor(name) {
        if (typeof (name) !== 'string' || name.length == 0) {
            throw new TypeError('Plugin name must be a string');
        }
        this.name = name;
        RASP.plugins[name] = this;
    }

    static check(checkPoint, checkParams, checkContext) {
        if (!RASP.checkPoints[checkPoint]) {
            throw new Error('Unknown check point name \'' + checkPoint + '\'');
        }
        const results = []
        for (const checkProcess of RASP.checkPoints[checkPoint]) {
            const result = checkProcess.plugin.make_result(checkProcess.func(checkParams, checkContext));
            if (result) {
                results.push(result);
            }
        }
        return results;
    }

    static sql_tokenize(query) {
        return tokenize(query, "sql")
    }

    static cmd_tokenize(query) {
        return tokenize(query, 'bash')
    }

    static request(config) {
        return request(config)
    }

    static get_jsengine() {
        return 'v8'
    }

    static get_version() {
        return version
    }

    register(checkPoint, checkProcess) {
        if (typeof (checkPoint) !== 'string' || checkPoint.length == 0) {
            throw new TypeError('Check point name must be a string');
        }
        if (checkPoints.indexOf(checkPoint) < 0) {
            this.log('Unknown check point name \'' + checkPoint + '\'');
            return;
        }
        if (typeof (checkProcess) !== 'function') {
            throw new TypeError('Check process must be a function');
        }
        RASP.checkPoints[checkPoint] = RASP.checkPoints[checkPoint] || []
        RASP.checkPoints[checkPoint].push({
            func: checkProcess,
            plugin: this
        });
    }

    log() {
        let len = arguments.length;
        let args = Array(len);
        for (let key = 0; key < len; key++) {
            args[key] = arguments[key];
        }
        console.log.apply(console, ['[' + this.name + ']'].concat(args));
    }

    make_result(result) {
        if (typeof result !== 'object' || result.action === 'ignore') {
            return;
        }
        if (typeof result.then !== 'function') {
            result.action = result.action || 'log';
            result.message = result.message || '';
            result.name = result.name || this.name;
            result.confidence = result.confidence || 0;
            return result;
        } else {
            return result.then(rst => {
                return this.make_result(rst)
            }).catch(err => {
                return this.make_result({
                    action: 'exception',
                    message: JSON.stringify(err)
                })
            })
        }
    }
};
RASP.plugins = {};
RASP.checkPoints = {};