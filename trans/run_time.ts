export function echo(p: any) {
    console.log(p)
}

const PAGE_SIZE = 65536 * 2; // 128KB 一页
const INITIAL_PAGES = 64; // 初始 64 页（128KB）

// 创建 Memory
export let memory = new WebAssembly.Memory({ initial: INITIAL_PAGES });
let buffer = new Uint8Array(memory.buffer);

// 堆管理状态
let heapStart = 0;
let freeList: number[] = []; // 简单的 free list，存释放回来的地址

/**
 * 分配指定大小的内存，返回指针
 * @param size - 要分配的字节数
 */
export function malloc(size: number): number {
    if (freeList.length > 0) {
        const ptr = freeList.pop()!;
        return ptr;
    }

    const ptr = heapStart;
    heapStart += size;

    // 内存不足？扩展 memory
    if (heapStart > buffer.byteLength) {
        const extraPages = Math.ceil((heapStart - buffer.byteLength) / PAGE_SIZE);
        memory.grow(extraPages);
        buffer = new Uint8Array(memory.buffer); // ✨✨ 重新绑定 buffer！
    }
    return ptr;
}

/**
 * 释放之前分配的内存
 * @param ptr - 要释放的地址
 */
export function free(ptr: number): void {
    freeList.push(ptr);
}

/**
 * 往指定地址写一个 32位整数
 * @param ptr - 地址
 * @param value - 值
 */
export function store(ptr: number, value: number): void {
    const view = new DataView(memory.buffer);
    view.setInt32(ptr, value, true); // little endian
}

/**
 * 从指定地址读一个 32位整数
 * @param ptr - 地址
 */
export function load(ptr: number): number {
    const view = new DataView(memory.buffer);
    return view.getInt32(ptr, true); // little endian
}