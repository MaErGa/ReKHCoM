/* Offline browser regression. Run with Node + Playwright available on NODE_PATH:
   node port/tools/art_review/test_pan.cjs
   Uses mocked HTTP responses; never opens the real review store. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {chromium} = require('playwright');
const web = path.join(__dirname, 'web');
const fixture = {
  id:'pan-test', title:'Pan regression fixture', version:'test', width:1200, height:900,
  image_url:'/fixture.svg', status:'pending', general:'', revision:1, doodles:[],
  provenance:'Offline fixture', annotations:[{id:'cloud-1',x:.1,y:.1,w:.2,h:.2,
    comment:'Preserve this cloud',category:'general',priority:'normal',expected:'',lesson:''}],
};

(async()=>{
  const browser = await chromium.launch({headless:true});
  try {
    const page = await browser.newPage({viewport:{width:1440,height:950}});
    const errors=[], writes=[];
    page.on('pageerror',e=>errors.push(e.message));
    await page.route('**/*',async route=>{
      const request=route.request(),pathname=new URL(request.url()).pathname;
      if(request.method()!=='GET')writes.push(pathname);
      if(pathname==='/api/state')return route.fulfill({json:{assets:[fixture],approvals:[],queue:[],app_version:'test'}});
      if(pathname==='/api/history')return route.fulfill({json:{events:[{at:'2026-10-24T12:00:00Z',review:fixture}]}});
      if(pathname==='/fixture.svg')return route.fulfill({contentType:'image/svg+xml',body:'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="900"><rect width="1200" height="900" fill="tan"/></svg>'});
      const file=pathname==='/'?'index.html':pathname.slice(1);
      if(['index.html','app.js','style.css'].includes(file))return route.fulfill({path:path.join(web,file)});
      return route.abort();
    });
    await page.goto('http://art-review.test/');
    await page.waitForFunction(()=>document.getElementById('title').textContent==='Pan regression fixture');
    await page.locator('#zoomNumber').fill('200%');
    await page.locator('#zoomNumber').press('Enter');
    const original=await page.evaluate(()=>JSON.stringify({annotations:current.annotations,doodles:current.doodles}));
    const point=async()=>{const b=await page.locator('#viewport').boundingBox();return {x:b.x+b.width/2,y:b.y+b.height/2};};
    const scroll=()=>page.evaluate(()=>({left:panViewport.scrollLeft,top:panViewport.scrollTop,
      maxLeft:panViewport.scrollWidth-panViewport.clientWidth,maxTop:panViewport.scrollHeight-panViewport.clientHeight,
      panning:panViewport.classList.contains('panning')}));
    const begin=async()=>{const p=await point();await page.mouse.move(p.x,p.y);await page.mouse.down({button:'right'});return p;};
    for(const mode of ['draw','pen','select','eraser','hand']){
      await page.locator('#'+mode).click();
      await page.evaluate(()=>{panViewport.scrollLeft=250;panViewport.scrollTop=200;});
      const p=await begin();
      assert.equal((await scroll()).panning,true,mode+' starts pan');
      await page.mouse.move(p.x-80,p.y-60,{steps:4});
      const moved=await scroll();assert.equal(moved.left,330,mode+' moves horizontally');assert.equal(moved.top,260);
      await page.mouse.up({button:'right'});
      assert.equal((await scroll()).panning,false);
      assert.equal(await page.locator('dialog[open]').count(),0,'Right drag opens no cloud editor');
      assert.equal(await page.evaluate(()=>JSON.stringify({annotations:current.annotations,doodles:current.doodles})),original);
    }
    // Exaggerated captured movement clamps to both edges even outside the viewport.
    let p=await begin();await page.mouse.move(-10000,-10000);let s=await scroll();
    assert.equal(s.left,s.maxLeft);assert.equal(s.top,s.maxTop);
    await page.mouse.move(10000,10000);s=await scroll();assert.equal(s.left,0);assert.equal(s.top,0);
    await page.mouse.up({button:'right'});
    // Cleanup paths must release the pointer and remove the grabbing cursor.
    for(const ending of ['pointercancel','lostpointercapture','blur']){
      await begin();
      await page.evaluate(ending=>{
        if(ending==='blur')window.dispatchEvent(new Event('blur'));
        else panViewport.dispatchEvent(new PointerEvent(ending,{pointerId:pan.pointerId,bubbles:true}));
      },ending);
      assert.equal((await scroll()).panning,false,ending+' clears cursor');
      assert.equal(await page.evaluate(()=>pan===null),true,ending+' clears state');
      await page.mouse.up({button:'right'});
    }
    // A right drag over an existing cloud must not activate its editor.
    await page.locator('#draw').click();
    await page.evaluate(()=>{panViewport.scrollLeft=200;panViewport.scrollTop=100;});
    const cloud=await page.locator('.cloud').boundingBox();
    await page.mouse.move(cloud.x+cloud.width/2,cloud.y+cloud.height/2);
    await page.mouse.down({button:'right'});await page.mouse.move(cloud.x+cloud.width/2-30,cloud.y+cloud.height/2-30);
    await page.mouse.up({button:'right'});
    assert.equal(await page.locator('dialog[open]').count(),0);
    // Existing middle button, Space + left, and hand + left gestures still pan.
    for(const gesture of ['middle','space','hand']){
      await page.locator('#'+(gesture==='hand'?'hand':'select')).click();
      await page.evaluate(()=>document.activeElement.blur()); // Space does not override focused form controls.
      await page.evaluate(()=>{panViewport.scrollLeft=250;panViewport.scrollTop=200;});
      p=await point();await page.mouse.move(p.x,p.y);
      if(gesture==='space')await page.keyboard.down('Space');
      const button=gesture==='middle'?'middle':'left';
      await page.mouse.down({button});await page.mouse.move(p.x-40,p.y-30);await page.mouse.up({button});
      if(gesture==='space')await page.keyboard.up('Space');
      s=await scroll();assert.equal(s.left,290,gesture);assert.equal(s.top,230,gesture);
    }
    const menuPrevented=await page.evaluate(()=>{
      const event=new MouseEvent('contextmenu',{bubbles:true,cancelable:true,button:2});
      document.getElementById('overlay').dispatchEvent(event);return event.defaultPrevented;
    });assert.equal(menuPrevented,true);
    // Historical review uses its normal UI and remains read-only while panning.
    await page.locator('#history').click();await page.locator('#historyEntries .history-entry').click();
    assert.equal(await page.locator('#historyBanner').isVisible(),true);
    await page.evaluate(()=>{panViewport.scrollLeft=250;panViewport.scrollTop=200;});
    p=await begin();await page.mouse.move(p.x-60,p.y-40);await page.mouse.up({button:'right'});
    s=await scroll();assert.equal(s.left,310);assert.equal(s.top,240);
    assert.equal(await page.evaluate(()=>JSON.stringify({annotations:current.annotations,doodles:current.doodles})),original);
    assert.deepEqual(writes,[],'Panning never saves or edits review data');
    assert.deepEqual(errors,[]);
    await page.locator('#fit').click();p=await begin();
    await page.mouse.move(p.x-100,p.y-100);await page.mouse.up({button:'right'});
    s=await scroll();assert.equal(s.left,0);assert.equal(s.top,0);
    console.log('PASS: right pan in draw/pen/select/eraser/hand/history; bounds, cleanup, context menu, no edits');
  } finally {await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
