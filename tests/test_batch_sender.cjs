const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const file='../docs/assets/batch-sender.js';
const H=fs.existsSync('docs/assets/batch-sender.js')?require(file):{};
const ticket='11111111-1111-4111-8111-111111111111';
const instance='22222222-2222-4222-8222-222222222222';
const item={problemNo:8,language:'c',codeSha256:'a'.repeat(64),problemUrl:'http://yoj.ruc.edu.cn/index.php/index/problem/detail/pno/8.html',submitEndpoint:'http://yoj.ruc.edu.cn/index.php/index/index/prob_submit.html',code:'int main(){}',submissionKind:'STANDARD_CODE'};
const expected=Object.fromEntries(['problemNo','language','codeSha256','problemUrl','submitEndpoint'].map(k=>[k,item[k]]));
function fixture(options={}) {
  assert.equal(typeof H.createSender,'function');
  const messages=[],submitted=[];
  const channel={postMessage:m=>messages.push(m),onmessage:null};
  const sender=H.createSender({ticket,instance,channel,loadItem:async()=>item,submit:x=>submitted.push(x),...options});
  const deliver=(type,fields={})=>channel.onmessage({data:{ticket,instance,type,...fields}});
  return {messages,submitted,sender,deliver};
}
test('sender independently loads expected published item and submits only after explicit one-shot SEND',async()=>{
  const f=fixture();assert.equal(f.messages[0].type,'ready');
  await f.deliver('prepare',{expected});assert.equal(f.messages.at(-1).type,'prepared');assert.equal(f.submitted.length,0);
  f.sender.left();assert.notEqual(f.messages.at(-1).type,'left');
  await f.deliver('send',{problemNo:8});await f.deliver('send',{problemNo:8});
  assert.deepEqual(f.submitted,[item]);assert.equal(f.messages.at(-1).type,'sending');
  f.sender.left();f.sender.left();assert.equal(f.messages.filter(m=>m.type==='left').length,1);
  assert.ok(f.messages.every(m=>m.ticket===ticket));
});
test('sender rejects changed metadata and preflight failures without a native POST',async()=>{
  for(const options of [{loadItem:async()=>({...item,codeSha256:'b'.repeat(64)})},{loadItem:async()=>{throw Error('hash failure');}}]) {
    const f=fixture(options);await f.deliver('prepare',{expected});await f.deliver('send',{problemNo:8});
    assert.equal(f.submitted.length,0);assert.equal(f.messages.at(-1).type,'failed');
  }
});
test('wrong ticket, wrong problem and premature SEND cannot dispatch',async()=>{
  const f=fixture();await f.deliver('prepare',{ticket:'wrong',expected});assert.equal(f.messages.length,1);
  await f.deliver('send',{problemNo:8});await f.deliver('prepare',{expected});assert.equal(f.submitted.length,0);
  const g=fixture();await g.deliver('prepare',{expected});await g.deliver('send',{problemNo:9});assert.equal(g.submitted.length,0);
});
test('failed native call emits failure and cannot later emit departure or retry',async()=>{
  const f=fixture({submit(){throw Error('native error');}});await f.deliver('prepare',{expected});await f.deliver('send',{problemNo:8});
  f.sender.left();await f.deliver('send',{problemNo:8});assert.equal(f.messages.at(-1).type,'failed');
});
test('sender form uses child document and keeps exact checked code, endpoint and language',()=>{
  assert.equal(typeof H.nativePost,'function');
  const sent=[];const doc={body:{append:f=>sent.push(f)},defaultView:{opener:{}},createElement:tag=>({tag,children:[],append(x){this.children.push(x);},submit(){this.called=true;}})};
  H.nativePost(doc,item);assert.equal(doc.defaultView.opener,null);assert.equal(sent.length,1);
  assert.equal(sent[0].target,'_self');assert.equal(sent[0].action,item.submitEndpoint);assert.equal(sent[0].called,true);
  assert.deepEqual(Object.fromEntries(sent[0].children.map(c=>[c.name,c.value])),{pid:'8',language:'c',code:item.code});
});
test('cancel during independent preflight cannot revive or submit the sender',async()=>{
  let release;const f=fixture({loadItem:()=>new Promise(r=>{release=r;})});
  const pending=f.deliver('prepare',{expected});await f.deliver('cancel');release(item);await pending;
  await f.deliver('send',{problemNo:8});assert.equal(f.submitted.length,0);assert.equal(f.messages.some(m=>m.type==='prepared'),false);
});
test('invalid ticket never creates a sender channel protocol',()=>{
  assert.throws(()=>H.createSender({ticket:'missing'}));
});

test('duplicated sender document cannot consume the selected instance commands',async()=>{
  const first=fixture(),duplicate=fixture({instance:'33333333-3333-4333-8333-333333333333'});
  for(const f of [first,duplicate])await f.deliver('prepare',{expected});
  for(const f of [first,duplicate])await f.deliver('send',{problemNo:8});
  assert.equal(first.submitted.length,1);assert.equal(duplicate.submitted.length,0);
});
