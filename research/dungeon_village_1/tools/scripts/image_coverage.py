"""固定 APK / Steam 容器和图像全量盘点；标准库、只读输入、仅输出清单。"""
from pathlib import Path
import hashlib, json, struct, zipfile, zlib, collections, csv

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'work/image-coverage-audit'
sha = lambda b: hashlib.sha256(b).hexdigest()
key = struct.pack('<11i', -1387743643,321849466,-380916995,1114766278,1209944503,138008561,-893766998,-1242421477,-1230126924,626883230,1684377624)
records, containers, errors = [], [], []
checks = 0
png_cache = {}
index_payloads = {}
def need(v, msg):
    global checks
    if not v: raise ValueError(msg)
    checks += 1

def image_info(b):
    if b.startswith(b'\x89PNG\r\n\x1a\n'):
        digest=sha(b)
        if digest in png_cache:return dict(png_cache[digest])
        p, chunks, idat, palette, transparency = 8, [], [], b'', b''
        while p < len(b):
            need(p+12 <= len(b), 'PNG chunk bounds')
            n = struct.unpack_from('>I',b,p)[0]; typ = b[p+4:p+8]
            need(p+12+n <= len(b), 'PNG payload bounds')
            data=b[p+8:p+8+n]
            need(zlib.crc32(typ+data)&0xffffffff == struct.unpack_from('>I',b,p+8+n)[0], 'PNG CRC')
            chunks.append(typ.decode('ascii'))
            if typ == b'IHDR':
                need(n == 13,'PNG IHDR')
                w,h,depth,color,comp,filt,interlace=struct.unpack('>IIBBBBB',data)
            if typ == b'IDAT': idat.append(data)
            if typ == b'PLTE':palette=data
            if typ == b'tRNS':transparency=data
            p += 12+n
            if typ == b'IEND': break
        need(chunks[0]=='IHDR' and chunks[-1]=='IEND','PNG structure end')
        decoded=zlib.decompress(b''.join(idat)); need(w>0 and h>0 and decoded,'PNG dimensions/zlib')
        info=dict(encoding='PNG',width=w,height=h,bit_depth=depth,color_type=color,interlace=interlace,
                    trailing_bytes=len(b)-p,trailing_sha256=sha(b[p:]) if p<len(b) else None,
                    validation='signature_chunks_crc_zlib_not_pixel_decoder')
        if interlace==0 and color in (2,3,6) and (depth==8 or color==3 and depth in (1,2,4)):
            channels={2:3,3:1,6:4}[color];rowlen=(w*channels*depth+7)//8;bpp=max(1,(channels*depth+7)//8)
            need(len(decoded)==h*(rowlen+1),'PNG decompressed row lengths')
            prev=bytearray(rowlen);rgba=bytearray();at=0
            for _ in range(h):
                filt=decoded[at];at+=1;row=bytearray(decoded[at:at+rowlen]);at+=rowlen
                need(0<=filt<=4,'PNG filter')
                if filt:
                    for j in range(rowlen):
                        a=row[j-bpp] if j>=bpp else 0;up=prev[j];c=prev[j-bpp] if j>=bpp else 0
                        if filt==1:v=a
                        elif filt==2:v=up
                        elif filt==3:v=(a+up)//2
                        else:
                            pa=abs(up-c);pb=abs(a-c);pc=abs(a+up-2*c)
                            v=a if pa<=pb and pa<=pc else up if pb<=pc else c
                        row[j]=(row[j]+v)&255
                if color==6:rgba.extend(row)
                elif color==2:
                    transparent=struct.unpack('>3H',transparency) if transparency else None
                    for j in range(0,rowlen,3):
                        rgb=tuple(row[j:j+3]);rgba.extend(rgb);rgba.append(0 if rgb==transparent else 255)
                else:
                    need(len(palette)%3==0 and palette,'PNG palette')
                    for j in range(w):
                        idx=(row[(j*depth)//8]>>(8-depth-(j*depth)%8))&((1<<depth)-1)
                        need(idx*3+3<=len(palette),'PNG palette index')
                        rgba.extend(palette[idx*3:idx*3+3]);rgba.append(transparency[idx] if idx<len(transparency) else 255)
                prev=row
            need(len(rgba)==w*h*4,'PNG RGBA dimensions')
            info.update(rgba_sha256=sha(rgba),rgba_bytes=len(rgba),validation='signature_chunks_crc_zlib_scanline_filters_palette_rgba')
        png_cache[digest]=info
        return dict(info)
    if b[:6] in (b'GIF87a',b'GIF89a'):
        return dict(encoding='GIF',width=int.from_bytes(b[6:8],'little'),height=int.from_bytes(b[8:10],'little'),validation='header_only')
    if b.startswith(b'\xff\xd8\xff'): return dict(encoding='JPEG',validation='signature_only')
    if b.startswith(b'RIFF') and b[8:12]==b'WEBP': return dict(encoding='WEBP',validation='signature_only')
    return None

def archive(b):
    p=0
    def u():
        nonlocal p
        need(p+4<=len(b),'archive header bounds'); v=int.from_bytes(b[p:p+4],'big');p+=4;return v
    fmt,size,count=u(),u(),u(); need(count<=100000,'archive count')
    names=[]
    for _ in range(count):
        n=u();need(0<n<=4096 and p+n<=len(b),'archive name bounds')
        names.append(b[p:p+n].decode('utf8'));p+=n
    need(len({n.lower() for n in names})==count,'archive duplicate names')
    offsets=[u() for _ in names]; declared=[u() for _ in names]
    need(p+count<=len(b),'archive flags');flags=b[p:p+count];p+=count
    need(len(b)-p==size,'archive data length')
    result=[]; spans=[]
    for i,name in enumerate(names):
        q=p+offsets[i];need(p<=q<=len(b)-4,'archive offset');n=int.from_bytes(b[q:q+4],'big')
        need(q+4+n<=len(b),'archive entry bounds');need(not(flags[i]&1 and declared[i]),'unsupported compression')
        spans.append((q,q+4+n));result.append((name,b[q+4:q+4+n],flags[i]))
    need(all(a[1]<=b[0] for a,b in zip(sorted(spans),sorted(spans)[1:])),'archive overlap')
    return fmt,result

def add_entry(source,container,entry,b,kind='entry',extra=None):
    r=dict(id=f'{source}:{container}:{entry}',source=source,container=container,entry=entry,kind=kind,bytes=len(b),sha256=sha(b))
    info=image_info(b)
    if info:r.update(info);r['kind']='image'
    elif entry.lower().endswith('.seb'):
        r['kind']='SEB'
        need(len(b)>=4,'SEB header')
        if b[0]&128:r['structural_validation']='unsupported_compressed_SEB'
        else:
            layers,frames=struct.unpack_from('>hh',b,0);at=4;refs=collections.Counter();parts=0;outside=[]
            need(0<=layers<=4096 and frames>=0,'SEB counts')
            for _ in range(layers):
                need(at+4<=len(b),'SEB layer bounds');count,tag=struct.unpack_from('>hH',b,at);at+=4
                need(0<=count<=100000 and at+count*20<=len(b),'SEB part bounds')
                for _ in range(count):
                    vals=struct.unpack_from('>10h',b,at);at+=20
                    if not 0<=vals[0]<frames:outside.append(vals[0])
                    refs[vals[1]]+=1
                parts+=count
            need(at==len(b),'SEB exact end')
            r.update(structural_validation='uncompressed_legacy_complete',layers=layers,frame_count=frames,
                     part_count=parts,image_ref_counts=dict(refs),keyframes_outside_nominal_count=outside)
    elif entry.lower().endswith('.inf'):
        r['kind']='INF';index_payloads.setdefault((source,container),{})[entry]=b
    if extra:r.update(extra)
    records.append(r);return r

def parse_archive(source,container,b,encrypted=True,resource_group=None,origin='archive'):
    decoded=bytes(v^key[i%len(key)] for i,v in enumerate(b)) if encrypted else b
    fmt,entries=archive(decoded)
    containers.append(dict(id=f'{source}:{container}',source=source,path=container,bytes=len(b),sha256=sha(b),kind='kairo_archive',format=fmt,entry_count=len(entries),origin=origin,resource_group=resource_group))
    for name,data,flags in entries:add_entry(source,container,name,data,extra={'flags':flags,'resource_group':resource_group,'archive_origin':origin})

def ico_images(container,name,b):
    """ICO 条目与 DIB 头盘点；不将未解码的 DIB 当作像素已核对。"""
    reserved,typ,count=struct.unpack_from('<HHH',b,0)
    need(reserved==0 and typ==1 and 0<count<=4096,'ICO header')
    need(6+count*16<=len(b),'ICO directory bounds')
    for i in range(count):
        at=6+i*16;w,h,colors,zero,planes,bits,n,offset=struct.unpack_from('<4B2H2I',b,at)
        need(6+count*16<=offset and offset+n<=len(b),'ICO frame bounds')
        frame=b[offset:offset+n]
        r=add_entry('EXE_PACKAGE',container,name+':frame='+str(i),frame,'ICO_frame',extra=dict(ico_width=w or 256,ico_height=h or 256))
        if r['kind']!='image':
            need(len(frame)>=40 and struct.unpack_from('<I',frame,0)[0]>=40,'ICO DIB header')
            dw,dh=struct.unpack_from('<ii',frame,4)
            need(abs(dw)==(w or 256) and abs(dh)==2*(h or 256),'ICO DIB dimensions')
            r.update(kind='image',encoding='DIB',width=abs(dw),height=abs(dh)//2,validation='ICO_directory_DIB_header_not_pixel_decode')

def managed_resources(p):
    """只读 IL2CPP 托管资源外壳，完整验证目录与连续载荷，不执行程序集。"""
    b=p.read_bytes();rel=p.relative_to(game).as_posix()
    need(len(b)>=8,'managed resource header')
    header,count=struct.unpack_from('<II',b,0);at=8;entries=[]
    need(4+header<=len(b) and count<=100000,'managed resource directory bounds')
    for _ in range(count):
        need(at+8<=4+header,'managed resource entry header')
        length,n=struct.unpack_from('<II',b,at);at+=8
        need(0<n<=4096 and at+n<=4+header,'managed resource name bounds')
        name=b[at:at+n].decode('utf8');at+=n;entries.append((name,length))
    need(at==4+header,'managed resource directory exact end')
    need(len({name for name,_ in entries})==count,'managed resource unique names')
    need(at+sum(length for _,length in entries)==len(b),'managed resource payload exact end')
    containers.append(dict(id='EXE:'+rel,source='EXE',path=rel,kind='IL2CPPManagedResources',bytes=len(b),sha256=sha(b),entry_count=count,header_bytes=4+header))
    for name,length in entries:
        payload=b[at:at+length]
        r=add_entry('EXE',rel,name,payload,'ManagedResource',extra=dict(offset=at,embedded_png_signature_count=payload.count(b'\x89PNG\r\n\x1a\n'),payload_format='uninterpreted'))
        at+=length
        if name in ('KairoLibrary.resource.kairolib.bytes','KairoLibrary.resource.langpack.bytes'):
            resource_group=name.split('.')[-2]
            parse_archive('EXE',rel+':'+name,payload,False,resource_group,'IL2CPP_managed')
            r['payload_format']='kairo_archive';r['archive_container_parsed']=True
        elif payload[:4]==b'\xce\xca\xef\xbe':
            r['payload_format']='dotnet_resources_magic_only'
        elif payload[:4]==b'\x00\x00\x01\x00':
            ico_images(rel,name,payload);r['payload_format']='ICO_directory_and_DIB_headers'

apk=ROOT/'maoxianmigongcun.apk'
need(sha(apk.read_bytes())=='1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5','fixed APK identity')
with zipfile.ZipFile(apk) as z:
    need(z.testzip() is None,'APK ZIP CRC')
    containers.append(dict(id='APK:maoxianmigongcun.apk',source='APK',path='maoxianmigongcun.apk',kind='ZIP',bytes=apk.stat().st_size,sha256=sha(apk.read_bytes()),entry_count=len(z.infolist())))
    for i in z.infolist():
        if i.is_dir():continue
        b=z.read(i)
        if i.filename.startswith('assets/') and i.filename.endswith('.dat'):
            try:parse_archive('APK',i.filename,b)
            except Exception as e:errors.append(dict(source='APK',container=i.filename,error=str(e),bytes=len(b),sha256=sha(b)))
        else:add_entry('APK','maoxianmigongcun.apk',i.filename,b,'ZIP_entry')

def serialized(p):
    b=p.read_bytes(); rel=p.relative_to(ROOT/'DungeonVillageEXE/KairoGames_Data').as_posix()
    need(len(b)>=48 and struct.unpack_from('>I',b,8)[0]==22,'Unity v22')
    data_start=struct.unpack_from('>Q',b,32)[0];pos=48
    def take(n):
        nonlocal pos
        need(0<=n and pos+n<=data_start,'Unity metadata bounds');v=b[pos:pos+n];pos+=n;return v
    def i32():return struct.unpack('<i',take(4))[0]
    end=b.index(0,pos);unity=b[pos:end].decode();pos=end+1
    platform=i32();trees=take(1)[0];types=[]
    for _ in range(i32()):
        cid=i32();take(1);script=struct.unpack('<h',take(2))[0]
        if cid==114:take(16)
        take(16)
        if trees:
            nodes=i32();strings=i32();need(0<=nodes<100000 and 0<=strings<10000000,'Unity tree counts')
            take(nodes*32+strings)
            deps=i32();need(0<=deps<100000,'Unity deps');take(deps*4)
        types.append(cid)
    count=i32();need(0<=count<=100000,'Unity object count');objs=[]
    for _ in range(count):
        pos=(pos+3)&~3
        pid,offset,size,tid=struct.unpack('<qqIi',take(24));offset+=data_start
        need(0<=tid<len(types) and data_start<=offset and offset+size<=len(b),'Unity object bounds')
        objs.append((str(pid),offset,size,types[tid]))
    containers.append(dict(id='EXE:'+rel,source='EXE',path=rel,kind='UnitySerializedFile',bytes=len(b),sha256=sha(b),unity=unity,platform=platform,type_tree=bool(trees),object_count=count,class_counts=dict(collections.Counter(x[3] for x in objs))))
    texture_dimensions={}
    sprite_references=[]
    for pid,offset,size,cid in objs:
        obj=b[offset:offset+size]
        r=dict(id=f'EXE:{rel}:pathID={pid}',source='EXE',container=rel,entry='pathID='+pid,kind='UnityObject',class_id=cid,bytes=size,sha256=sha(obj),offset=offset)
        if cid in (28,49,213):
            n=struct.unpack_from('<i',obj,0)[0];need(0<=n<=size-4,'Unity name bounds')
            r['name']=obj[4:4+n].decode('utf8');q=(4+n+3)&~3
            if cid==49:
                length=struct.unpack_from('<i',obj,q)[0];q+=4;need(0<=length<=size-q,'TextAsset bounds')
                payload=obj[q:q+length];r.update(payload_bytes=length,payload_sha256=sha(payload),kind='TextAsset')
                info=image_info(payload)
                if info:r.update(info);r['kind']='TextAsset_image'
                try:
                    # 已核Kairo文本容器；其它TextAsset只枚举身份，不猜格式。
                    if r['name'] in ['xls','map','image','human','monster','weapon','common','common2','effect','event','load','system','title','kairolib','lineup','amazonbnr','test','language']:
                        parse_archive('EXE',rel+':'+pid+':'+r['name'],payload,r['name']!='kairolib',r['name'],'Unity_TextAsset')
                except Exception as e:errors.append(dict(source='EXE',container=rel+':'+pid+':'+r['name'],error=str(e),bytes=length,sha256=sha(payload)))
                r['embedded_png_signature_count']=payload.count(b'\x89PNG\r\n\x1a\n')
                r['archive_container_parsed']=any(c['source']=='EXE' and c['path']==rel+':'+pid+':'+r['name'] for c in containers)
            elif cid==28:
                r['kind']='Texture2D'
                need(q+88<=size,'Texture2D metadata bounds')
                w,h,total,stripped,fmt,mips=struct.unpack_from('<6i',obj,q+8)
                need(0<=w<=65536 and 0<=h<=65536 and total>=0 and mips>=0,'Texture2D dimensions')
                r.update(width=w,height=h,texture_format=fmt,mip_count=mips,complete_image_size=total,
                         mips_stripped=stripped,validation='v2021_3_structural_payload_bounds_not_pixel_decode')
                platformlen=struct.unpack_from('<i',obj,q+80)[0];need(0<=platformlen<size-q-84,'Texture platform bounds')
                at=(q+84+platformlen+3)&~3
                length=struct.unpack_from('<i',obj,at)[0];at+=4;need(0<=length<=size-at,'Texture inline bounds')
                inline=obj[at:at+length];at=(at+length+3)&~3
                need(at+16<=size,'Texture stream info bounds')
                so,ss,pn=struct.unpack_from('<QIi',obj,at);at+=16;need(0<=pn<=size-at,'Texture stream name bounds')
                stream=obj[at:at+pn].decode('utf8');at=(at+pn+3)&~3
                need(at==size,'Texture no unexplained tail')
                if ss:
                    streamfile=(p.parent/stream).resolve()
                    need(streamfile.is_relative_to(game.resolve()) and streamfile.is_file(),'Texture stream source')
                    sb=streamfile.read_bytes();need(so+ss<=len(sb),'Texture external stream bounds')
                    payload=sb[so:so+ss]
                    r.update(storage='external',stream_file=streamfile.relative_to(game.resolve()).as_posix(),stream_offset=so)
                else:payload=inline;r['storage']='inline'
                r.update(payload_bytes=len(payload),payload_sha256=sha(payload),empty_runtime_texture=(w==0 or h==0))
                need(total==len(payload),'Texture declared payload length')
                texture_dimensions[pid]=(w,h)
            else:
                need(q+16<=len(obj),'Sprite rect bounds')
                rect=list(struct.unpack_from('<4f',obj,q));at=q+84
                need(at+4<=size,'Sprite tags header');nt=struct.unpack_from('<i',obj,at)[0];at+=4
                need(0<=nt<100000,'Sprite tag count')
                for _ in range(nt):
                    n=struct.unpack_from('<i',obj,at)[0];at+=4;need(0<=n<=size-at,'Sprite tag bounds');at=(at+n+3)&~3
                need(at+36<=size,'Sprite texture pointers bounds')
                af,aid,tf,tid,alf,alid=struct.unpack_from('<iqiqiq',obj,at)
                r.update(kind='Sprite',rect=rect,width=rect[2],height=rect[3],sprite_atlas={'file_id':af,'path_id':str(aid)},
                         texture={'file_id':tf,'path_id':str(tid)},alpha_texture={'file_id':alf,'path_id':str(alid)},
                         validation='v2021_3_rect_texture_pointer_not_mesh_pixel_decode')
                if tf==0 and tid:
                    need(any(x[0]==str(tid) and x[3]==28 for x in objs),'Sprite local texture class')
                    sprite_references.append((r,str(tid),rect))
        records.append(r)
    for r,tid,rect in sprite_references:
        w,h=texture_dimensions[tid]
        need(rect[0]>=0 and rect[1]>=0 and rect[2]>0 and rect[3]>0 and rect[0]+rect[2]<=w and rect[1]+rect[3]<=h,'Sprite rect within texture')
        r['texture_record']='EXE:'+rel+':pathID='+tid

game=ROOT/'DungeonVillageEXE/KairoGames_Data'
input_files=[]
for p in sorted(game.rglob('*')):
    if not p.is_file():continue
    b=p.read_bytes(); rel=p.relative_to(game).as_posix()
    input_files.append(dict(path=rel,bytes=len(b),sha256=sha(b),png_signature_count=b.count(b'\x89PNG\r\n\x1a\n')))
    if len(b)>=48 and b[8:12]==b'\x00\x00\x00\x16':
        try:serialized(p)
        except Exception as e:errors.append(dict(source='EXE',container=rel,error=str(e),bytes=len(b),sha256=sha(b)))
    elif rel.startswith('il2cpp_data/Resources/') and rel.endswith('-resources.dat'):
        try:managed_resources(p)
        except Exception as e:errors.append(dict(source='EXE',container=rel,error=str(e),bytes=len(b),sha256=sha(b)))
    elif image_info(b):add_entry('EXE','loose_files',rel,b)

def pe_images(p):
    b=p.read_bytes()
    if len(b)<64 or b[:2]!=b'MZ':return
    pe=struct.unpack_from('<I',b,60)[0];need(pe+24<=len(b) and b[pe:pe+4]==b'PE\0\0','PE identity')
    sections,opt_size=struct.unpack_from('<H',b,pe+6)[0],struct.unpack_from('<H',b,pe+20)[0]
    opt=pe+24;magic=struct.unpack_from('<H',b,opt)[0];dd=opt+(96 if magic==0x10b else 112)
    rr,rs=struct.unpack_from('<II',b,dd+16)
    if not rr:return
    sects=[]
    for i in range(sections):
        at=opt+opt_size+i*40;vs,va,raw,off=struct.unpack_from('<4I',b,at+8);sects.append((va,max(vs,raw),off))
    def rva(v):
        for va,size,off in sects:
            if va<=v<va+size:return off+v-va
        raise ValueError('PE resource RVA')
    base=rva(rr); rel=p.relative_to(ROOT/'DungeonVillageEXE').as_posix(); leaves=[]
    def walk(at,keys):
        need(base<=at and at+16<=len(b),'PE resource directory')
        named,ids=struct.unpack_from('<HH',b,at+12)
        for i in range(named+ids):
            name,value=struct.unpack_from('<II',b,at+16+i*8)
            if name&0x80000000:
                np=base+(name&0x7fffffff);n=struct.unpack_from('<H',b,np)[0];keyname=b[np+2:np+2+n*2].decode('utf-16-le')
            else:keyname=name
            if value&0x80000000:
                need(len(keys)<4,'PE resource depth');walk(base+(value&0x7fffffff),keys+[keyname])
            else:
                dp=base+value;v,n=struct.unpack_from('<II',b,dp);off=rva(v)
                need(off+n<=len(b),'PE resource data bounds');leaves.append((keys+[keyname],b[off:off+n]))
    walk(base,[])
    visual=[(keys,data) for keys,data in leaves if keys[0] in (1,2,3,12,14,'PNG')]
    containers.append(dict(id='EXE_PACKAGE:'+rel,source='EXE_PACKAGE',path=rel,kind='PE_resources',bytes=len(b),sha256=sha(b),resource_leaf_count=len(leaves),visual_resource_count=len(visual)))
    for keys,data in visual:
        r=add_entry('EXE_PACKAGE',rel,'/'.join(map(str,keys)),data,'PE_visual_resource')
        r['resource_type']=keys[0]
        if r['kind']!='image' and keys[0] in (1,2,3) and len(data)>=12:
            dib=data[4:] if keys[0]==1 else data
            header=struct.unpack_from('<I',dib)[0]
            if header>=40 and len(dib)>=40:
                w,h=struct.unpack_from('<ii',dib,4)
                r.update(encoding='DIB',width=abs(w),height=abs(h)//2 if keys[0] in (1,3) else abs(h),validation='PE_directory_DIB_header_not_pixel_decode',kind='image')

package_input_files=[]
for p in sorted((ROOT/'DungeonVillageEXE').rglob('*')):
    if p.is_file():
        b=p.read_bytes()
        package_input_files.append(dict(path=p.relative_to(ROOT/'DungeonVillageEXE').as_posix(),bytes=len(b),sha256=sha(b),png_signature_count=b.count(b'\x89PNG\r\n\x1a\n')))
    if p.is_file() and p.suffix.lower() in ('.exe','.dll'):
        try:pe_images(p)
        except Exception as e:errors.append(dict(source='EXE_PACKAGE',container=p.relative_to(ROOT/'DungeonVillageEXE').as_posix(),error=str(e)))

def group(r):
    if r.get('resource_group'):return r['resource_group']
    if r['source']=='APK' and r['container'].startswith('assets/'):
        return Path(r['container']).stem
    if r['source']=='EXE' and r['container'].count(':')==2:
        return r['container'].split(':')[-1]
    return None
apk_entries={}
for r in records:
    r['group']=group(r)
    if r['source']=='APK' and r['group']:apk_entries[(r['group'],r['entry'])]=r
published=[]
with (ROOT/'assets/MANIFEST.tsv').open(encoding='utf8',newline='') as f:
    for row in csv.DictReader(f,delimiter='\t'):
        if not row['source_archive']:continue
        g=Path(row['source_archive']).stem;e=row['source_entry']
        r=apk_entries.get((g,e));need(r is not None and r['sha256']==row['source_sha256'],'published entry direct APK equality')
        p=ROOT/'assets/original'/g/e;need(p.is_file() and sha(p.read_bytes())==r['sha256'],'published local file direct APK equality')
        r['published_original_path']='assets/original/'+g+'/'+e
        published.append(r['id'])
need(len(published)==761 and len(set(published))==761,'frozen publication denominator')
for r in records:
    if r['source']=='EXE' and r['group']:
        e=r['entry'];parts=e.split('/');lang=None
        if parts[0].endswith('.lproj'):lang=parts[0];e='/'.join(parts[1:])
        r['language_variant']=lang
        candidate=apk_entries.get((r['group'],e))
        r['apk_name_candidate']=candidate['id'] if candidate else None
        r['apk_same_name_bytes_equal']=bool(candidate and r['sha256']==candidate['sha256'])
        r['apk_same_name_pixels_equal']=bool(candidate and r.get('rgba_sha256') and r.get('rgba_sha256')==candidate.get('rgba_sha256'))
records_by_container=collections.defaultdict(dict)
for r in records:records_by_container[(r['source'],r['container'])][r['entry']]=r
for (src,container),files in index_payloads.items():
    for entry,b in files.items():
        if Path(entry).name not in ('img.inf','seb.inf'):continue
        record=records_by_container[(src,container)][entry];parent=Path(entry).parent.as_posix();prefix='' if parent=='.' else parent+'/'
        targets=[]
        for line in b.decode('utf8').splitlines():
            if not line.strip():continue
            fields=line.split('\t');idx=int(fields[0]) if len(fields)>1 and fields[0].isdigit() else len(targets)
            filename=fields[1] if len(fields)>1 and fields[0].isdigit() else fields[0]
            filename=filename.replace('.gif','.png') if Path(entry).name=='img.inf' else filename
            filename_parts=filename.split(',');filename=filename_parts[0]
            dest=records_by_container[(src,container)].get(prefix+filename)
            if dest is None:dest=records_by_container[(src,container)].get(filename)
            targets.append(dict(index=idx,filename=filename,record=dest['id'] if dest else None,
                modifiers=filename_parts[1:]+(fields[2:] if len(fields)>1 and fields[0].isdigit() else fields[1:])))
        record['index_targets']=targets
aliases=[]
for src in ('APK','EXE'):
    groups=collections.defaultdict(list)
    for r in records:
        if r['source']==src and r['kind']=='image':groups[r['sha256']].append(r['id'])
    aliases.extend(dict(source=src,sha256=h,records=ids) for h,ids in groups.items() if len(ids)>1)
summary=dict(checks=checks,record_count=len(records),errors=errors,published_original_files=len(published),
 source_kinds={src:dict(collections.Counter(r['kind'] for r in records if r['source']==src)) for src in ['APK','EXE','EXE_PACKAGE']},
 image_unique_bytes={src:len({r['sha256'] for r in records if r['source']==src and r['kind']=='image'}) for src in ['APK','EXE']},
 apk_not_published_images=[r['id'] for r in records if r['source']=='APK' and r['kind']=='image' and 'published_original_path' not in r],
 exe_image_comparison=dict(collections.Counter('same_name_equal' if r.get('apk_same_name_bytes_equal') else 'same_name_different' if r.get('apk_name_candidate') else 'no_name_candidate' for r in records if r['source']=='EXE' and r['kind']=='image')))
summary['exe_pixel_comparison']=dict(collections.Counter('same_name_equal_pixels' if r.get('apk_same_name_pixels_equal') else 'same_name_different_pixels' if r.get('apk_name_candidate') else 'no_name_candidate' for r in records if r['source']=='EXE' and r['kind']=='image'))
summary['png_rgba_verified']={src:sum(1 for r in records if r['source']==src and r.get('rgba_sha256')) for src in ['APK','EXE','EXE_PACKAGE']}
summary['seb_structure']={src:dict(collections.Counter(r.get('structural_validation') for r in records if r['source']==src and r['kind']=='SEB')) for src in ['APK','EXE']}
summary['unity_texture_formats']=dict(collections.Counter(str(r['texture_format']) for r in records if r['kind']=='Texture2D'))
summary['unity_empty_runtime_textures']=sum(r.get('empty_runtime_texture',False) for r in records if r['kind']=='Texture2D')
summary['image_alias_groups']=len(aliases)
summary['image_alias_surplus']=sum(len(a['records'])-1 for a in aliases)
summary['inf_unresolved_targets']=sum(a['record'] is None for r in records for a in r.get('index_targets',[]))
summary['nominal_seb_keyframe_diagnostics']={src:sum(bool(r.get('keyframes_outside_nominal_count')) for r in records if r['source']==src and r['kind']=='SEB') for src in ['APK','EXE']}
summary['exe_archive_images_by_origin']=dict(collections.Counter(r.get('archive_origin','non_archive') for r in records if r['source']=='EXE' and r['kind']=='image'))
summary['exe_managed_payloads']=[dict(container=r['container'],name=r['entry'],bytes=r['bytes'],format=r['payload_format'],png_signatures=r['embedded_png_signature_count']) for r in records if r['kind']=='ManagedResource']
summary['containers_by_source_kind']={src:dict(collections.Counter(c['kind'] for c in containers if c['source']==src)) for src in ['APK','EXE','EXE_PACKAGE']}
summary['image_encodings_by_source']={src:dict(collections.Counter(r['encoding'] for r in records if r['source']==src and r['kind']=='image')) for src in ['APK','EXE','EXE_PACKAGE']}
summary['exe_package_file_count']=len(package_input_files)
summary['unity_class_counts']=dict(sum((collections.Counter(c.get('class_counts',{})) for c in containers if c['kind']=='UnitySerializedFile'),collections.Counter()))
summary['textassets_without_kairo_archive']=[dict(name=r['name'],bytes=r['payload_bytes'],png_signatures=r['embedded_png_signature_count']) for r in records if r['kind']=='TextAsset' and not r['archive_container_parsed']]
need(len({r['id'] for r in records})==len(records),'inventory unique record IDs')
need(len({c['id'] for c in containers})==len(containers),'inventory unique container IDs')
summary['checks']=checks
OUT.mkdir(parents=True,exist_ok=True)
result=dict(schema_version=1,scope='full_container_identity_inventory_not_code_consumer_certification',
 analyzer=dict(path='tools/scripts/image_coverage.py',sha256=sha(Path(__file__).read_bytes())),
 input_apk=dict(path='maoxianmigongcun.apk',bytes=apk.stat().st_size,sha256=sha(apk.read_bytes())),
 containers=containers,exe_input_files=input_files,exe_package_input_files=package_input_files,records=records,image_alias_groups=aliases,summary=summary)
(ROOT/'assets/IMAGE_COVERAGE.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
(OUT/'SUMMARY.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
# 初次探针清单已被正式同职责清单替代；仅删除本脚本自己生成的冗余JSON。
if (OUT/'INVENTORY.json').is_file():(OUT/'INVENTORY.json').unlink()
print(json.dumps(summary,ensure_ascii=False,indent=2))
