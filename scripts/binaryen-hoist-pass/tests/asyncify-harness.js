// Minimal Asyncify harness: drive one unwind/rewind through $vt and check it yields 50.
const fs = require('fs');
const path = process.argv[2];
const BUF = 16, STACK = 1024, STACK_END = 16384;
let inst, rewinding = false, pending = 0;

const imports = {
  env: {
    sleep: (ms) => {
      if (!rewinding) {
        inst.exports.asyncify_start_unwind(BUF);
        pending = ms;
        return 0; // dummy; value is discarded as we unwind
      } else {
        inst.exports.asyncify_stop_rewind();
        rewinding = false;
        return pending; // real value supplied on rewind
      }
    },
  },
};

const mod = new WebAssembly.Module(fs.readFileSync(path));
inst = new WebAssembly.Instance(mod, imports);
// asyncify buffer struct: [current, end]
const mem = new Int32Array(inst.exports.memory.buffer);
mem[BUF >> 2] = STACK;
mem[(BUF + 4) >> 2] = STACK_END;

inst.exports.vt(); // runs, throws, catch calls sleep -> starts unwind, returns
inst.exports.asyncify_stop_unwind();
inst.exports.asyncify_start_rewind(BUF);
rewinding = true;
const r = inst.exports.vt(); // rewinds into the catch handler; sleep returns 50; vt completes
console.log('vt result =', r);
process.exit(r === 50 ? 0 : 1);
