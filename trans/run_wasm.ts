// run the main function in output.wasm
import fs from "node:fs"
import {sys_echo} from "./run_time";

async function runWasm() {
    try {
        // 加载wasm文件
        const wasmBuffer = fs.readFileSync('./output.wasm');
        const wasmModule = await WebAssembly.compile(wasmBuffer);
        
        // 实例化模块
        const imports = {
            "runtime": {
                "echo": sys_echo
            }
        };
        const instance = await WebAssembly.instantiate(wasmModule, imports);
        
        // 调用c_main函数
        const cMain = instance.exports.c_main as CallableFunction;
        if (typeof cMain === 'function') {
            cMain();
            console.log('c_main函数执行成功');
        } else {
            console.error('在wasm模块中找不到c_main函数');
        }
    } catch (error) {
        console.error('执行wasm时出错:', error);
    }
}

// 执行
runWasm();