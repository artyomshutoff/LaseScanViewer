'use strict';
window.LaseManualCalculator = function(api, getScan) {
 const $=id=>document.getElementById(id), dialog=$('manual'), tbody=$('manualRows');
 let rows=Array.from({length:5},()=>['','','']), unit='m', result=null, serial=0, timer, converting=false;
 const format=(v,n=3)=>Number(v).toLocaleString('ru-RU',{minimumFractionDigits:n,maximumFractionDigits:n});
 const volume=v=>v==null?'—':format(v)+' м³';
 function compare(){
  const scan=getScan(), valid=result?.valid, available=Number.isFinite(scan?.m3);
  $('manualPreliminary').textContent=available&&scan.provisional?'Предварительная оценка':'';
  $('manualScan').textContent=available?volume(scan.m3):'—';
  $('manualDifference').textContent=valid&&available?`${scan.m3-result.mean>=0?'+':''}${format(scan.m3-result.mean)} м³ / ${(scan.m3/result.mean-1)>=0?'+':''}${format((scan.m3/result.mean-1)*100,2)}%`:'—';
  $('manualAbsolute').textContent=valid&&available?`Абсолютное расхождение: ${format(Math.abs((scan.m3/result.mean-1)*100),2)}%`:!valid?'Заполните корректные обмеры для сравнения.':'Сначала рассчитайте объём скана.';
 }
 function clear(){result=null;for(const id of ['manualMean','manualMin','manualMax'])$(id).textContent='—';$('manualDimensions').textContent='Средние размеры: —';compare();}
 function paint(data){
  result=data;for(const [id,key] of [['manualMean','mean'],['manualMin','minimum'],['manualMax','maximum']])$(id).textContent=volume(data[key]);
  $('manualDimensions').textContent='Средние размеры: '+(data.dimensions?data.dimensions.map(v=>format(v)).join(' × ')+' м':'—');
  $('manualStatus').textContent=data.invalidRow?`Строка ${data.invalidRow}: введите три положительных размера.`:data.count?`Заполнено ${data.count} из ${rows.length} · пустые строки не учитываются`:'Введите размеры хотя бы в одной строке.';
  $('manualStatus').classList.toggle('error',!!data.invalidRow);
  [...tbody.children].forEach((tr,i)=>{tr.classList.toggle('invalid',i+1===data.invalidRow);tr.querySelector('.rowVolume').textContent=volume(data.volumes[i]);});compare();
 }
 function body(to){const form=new URLSearchParams({count:rows.length,unit});if(to)form.set('to',to);rows.forEach((row,i)=>row.forEach((text,j)=>form.set(`r${i}c${j}`,text)));return form;}
 async function calculate(){const request=++serial;try{const data=await(await api('manual',{method:'POST',body:body()})).json();if(request===serial)paint(data);}catch(e){if(request===serial){clear();$('manualStatus').textContent=e.message;$('manualStatus').classList.add('error');}}}
 function schedule(){clearTimeout(timer);++serial;clear();$('manualStatus').textContent='Расчёт обмеров…';timer=setTimeout(calculate,100);}
 function rebuild(){
  tbody.replaceChildren();rows.forEach((row,i)=>{
   const tr=document.createElement('tr'), number=document.createElement('td');number.textContent=i+1;tr.append(number);
   row.forEach((value,j)=>{const td=document.createElement('td'), input=document.createElement('input');input.type='text';input.inputMode='decimal';input.maxLength=64;input.value=value;input.setAttribute('aria-label',`${['Длина','Ширина','Высота'][j]}, обмер ${i+1}`);input.disabled=converting;input.oninput=()=>{rows[i][j]=input.value;schedule();};td.append(input);tr.append(td);});
   const cell=document.createElement('td');cell.className='rowVolume';cell.textContent='—';tr.append(cell);
   const actions=document.createElement('td'), remove=document.createElement('button');remove.type='button';remove.className='removeMeasurement';remove.textContent='Удалить';remove.setAttribute('aria-label',`Удалить обмер ${i+1}`);remove.disabled=rows.length===1||converting;remove.onclick=()=>{rows.splice(i,1);rebuild();schedule();};actions.append(remove);tr.append(actions);tbody.append(tr);
  });$('manualAdd').disabled=rows.length>=200||converting;
  for(const [id,name] of [['manualLength','Длина'],['manualWidth','Ширина'],['manualHeight','Высота']])$(id).textContent=name+(unit==='m'?', м':', см');
 }
 $('manualButton').onclick=()=>{dialog.showModal();rebuild();dialog.scrollTop=0;tbody.closest('.manualTable').scrollTop=0;calculate();};
 $('manualClose').onclick=()=>dialog.close();
 $('manualAdd').onclick=()=>{rows.push(['','','']);rebuild();schedule();tbody.lastElementChild.querySelector('input').focus();};
 $('manualUnits').onchange=async()=>{
  const target=$('manualUnits').value;if(target===unit)return;clearTimeout(timer);const request=++serial;let error='';converting=true;rebuild();$('manualUnits').disabled=true;
  try{const data=await(await api('manual',{method:'POST',body:body(target)})).json();if(request===serial){rows=data.rows;unit=data.unit;result=data;}}
  catch(e){$('manualUnits').value=unit;error=e.message;}
  finally{converting=false;$('manualUnits').disabled=false;rebuild();if(result)paint(result);if(error){$('manualStatus').textContent=error;$('manualStatus').classList.add('error');}}
 };
 return {refresh:()=>{if(dialog.open)compare();}};
};
