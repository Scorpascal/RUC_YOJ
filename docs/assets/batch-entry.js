/* Cross-tab coordination of one explicit batch, without accounts or source code. */
(function(root,factory){if(typeof module==='object'&&module.exports)module.exports=factory();else root.YOJBatchEntry=factory();})(typeof globalThis!=='undefined'?globalThis:this,()=>{
  'use strict';
  function create(storage,locks,id,numbers){
    const key='yoj-batch-entry-v1:'+id;let active=false;
    function read(){
      const text=storage.getItem(key);if(text===null)return {mode:'',selection:numbers,attempted:[]};
      let row;try{row=JSON.parse(text);}catch(_){throw Error('批次共享记录损坏，未发送。');}
      if(!row||!['cross','inpage'].includes(row.mode)||JSON.stringify(row.selection)!==JSON.stringify(numbers)||!Array.isArray(row.attempted)||new Set(row.attempted).size!==row.attempted.length||row.attempted.some(n=>!numbers.includes(n)))throw Error('批次共享记录不一致，未发送。');
      return row;
    }
    async function run(mode,fn){
      if(!locks?.request)throw Error('浏览器缺少跨页批次保护，未发送；请使用新版Safari或Chrome。');
      return locks.request(key,{ifAvailable:true},async lock=>{
        if(!lock)throw Error('此批次正在另一个页面处理，请核对原站，勿重复启动。');
        const row=read();if(row.mode&&row.mode!==mode)throw Error('此批次已绑定另一执行方式，请核对原站后明确新建批次。');
        row.mode=mode;storage.setItem(key,JSON.stringify(row));active=true;
        try{return await fn();}finally{active=false;}
      });
    }
    function remember(n){
      if(!active)throw Error('批次控制权不可确认，未发送。');
      const row=read();if(!numbers.includes(n)||row.attempted.includes(n))throw Error('此批次题目已尝试，禁止自动重发。');
      row.attempted.push(n);storage.setItem(key,JSON.stringify(row));
    }
    return {run,remember,mode:()=>read().mode,attempted:()=>new Set(read().attempted)};
  }
  return Object.freeze({create});
});
