const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const H=fs.existsSync('docs/assets/inpage-batch-core.js')?require('../docs/assets/inpage-batch-core.js'):{};
const id='11111111-1111-4111-8111-111111111111';
const rows=[8,9].map(problemNo=>({problemNo,codeSha256:'a'.repeat(64),language:'cpp',problemUrl:`http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/${problemNo}.html`,submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',code:'int main(){}'}));
function fixture(options={}) {
 assert.equal(typeof H.createRunner,'function');
 const posts=[],remembered=[],states=[];let clock=0;
 const runner=H.createRunner(rows,{attempted:()=>new Set(),remember:n=>remembered.push(n),guard:()=>{},preflight:async()=>{},baseline:async()=>100,
  post:async item=>posts.push({n:item.problemNo,at:clock}),receipt:async(item)=>({submissionNo:100+item.problemNo,problemNo:item.problemNo,status:'Accepted'}),
  pause:async ms=>{clock+=ms;},onChange:s=>states.push(s),...options});
 return {runner,posts,remembered,states};
}
test('handoff keeps exact approved public identity and rejects malformed or changed items',()=>{
 assert.equal(typeof H.makeHandoff,'function');const p=H.makeHandoff(id,rows);assert.deepEqual(H.parseHandoff(JSON.stringify(p)).items,p.items);
 assert.throws(()=>H.parseHandoff(JSON.stringify({...p,items:[p.items[0],p.items[0]]})));
 assert.throws(()=>H.matchItems(p,[{...rows[0],codeSha256:'b'.repeat(64)},rows[1]]));
 assert.equal(JSON.stringify(p).includes('int main'),false);
});
test('one start submits entire batch with receipts and pacing; duplicate start never resends',async()=>{
 const f=fixture();await f.runner.start();await f.runner.start();assert.deepEqual(f.remembered,[8,9]);assert.equal(f.posts.length,2);
 assert.ok(f.posts[1].at-f.posts[0].at>=3000);assert.equal(f.runner.snapshot().phase,'DONE');
 assert.deepEqual(f.runner.snapshot().rows.map(r=>r.receipt.submissionNo),[108,109]);
});
test('missing receipt or failing POST stops without retry or next item',async()=>{
 for(const options of [{receipt:async()=>{throw Error('unknown');}},{post:async()=>{throw Error('network');}}]) {
  const f=fixture(options);await f.runner.start();await f.runner.start();assert.deepEqual(f.remembered,[8]);
  assert.equal(f.runner.snapshot().phase,'STOPPED');assert.deepEqual(f.runner.snapshot().rows.map(r=>r.state),['UNCERTAIN','WAITING']);
 }
});
test('Stop during async preflight or polling never sends another item',async()=>{
 let release;const f=fixture({preflight:()=>new Promise(r=>{release=r;})});const pending=f.runner.start();await new Promise(r=>setImmediate(r));f.runner.stop();release();await pending;
 assert.equal(f.posts.length,0);assert.equal(f.remembered.length,0);
 const g=fixture({receipt:async()=>{g.runner.stop();return {submissionNo:108};}});await g.runner.start();assert.equal(g.posts.length,1);
});
test('prior attempts skipped; storage failure and lost lease fail before any POST',async()=>{
 const f=fixture({attempted:()=>new Set([8])});await f.runner.start();assert.deepEqual(f.posts.map(p=>p.n),[9]);
 for(const options of [{remember(){throw Error('storage');}},{guard(){throw Error('lease');}}]){const g=fixture(options);await g.runner.start();assert.equal(g.posts.length,0);}
});
test('batch execution mode persists and cannot switch silently',()=>{
 const S=require('../docs/assets/batch-session.js'),data=new Map(),storage={getItem:k=>data.get(k)||null,setItem:(k,v)=>data.set(k,v)};
 const ledger=S.createLedger(storage,id,[8,9]);ledger.claimMode('inpage');
 const resumed=S.createLedger(storage,id,[8,9],{resume:true});assert.equal(resumed.mode(),'inpage');assert.throws(()=>resumed.claimMode('cross'));
 resumed.claimMode('inpage');assert.deepEqual([...resumed.attempted()],[]);
});
