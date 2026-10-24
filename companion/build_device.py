"""Build our own Max patch/device container. No third-party device content is copied."""
from pathlib import Path
import json, struct, time
ROOT = Path(__file__).resolve().parent

def patch():
    boxes, lines = [], []
    def box(id, cls, text='', rect=(0,0,100,22), presentation=None, **extra):
        value={'id':id,'maxclass':cls,'patching_rect':list(rect),**extra}
        if text: value['text']=text
        if presentation: value.update(presentation=1,presentation_rect=list(presentation))
        boxes.append({'box':value})
    def link(a,b,port=0,inlet=0): lines.append({'patchline':{'source':[a,port],'destination':[b,inlet]}})
    box('midi-in','newobj','midiin',(20,260,60,22));box('midi-out','newobj','midiout',(20,310,60,22));link('midi-in','midi-out')
    box('ready','newobj','live.thisdevice',(130,260,100,22));box('init','message','init',(130,290,40,22));link('ready','init')
    box('defer','newobj','deferlow',(200,330,70,22));box('api','newobj','js bridge-max.js',(200,370,130,22));link('init','defer');link('defer','api')
    box('node','newobj','node.script bridge-node.js @autostart 1 @defer 1',(410,370,360,22));link('api','node');link('node','defer')
    box('metro','newobj','qmetro 300',(260,260,90,22));box('on','message','1',(260,230,30,22));box('poll','message','poll',(260,300,40,22));link('ready','on');link('on','metro');link('metro','poll');link('poll','defer')
    box('title','comment','PIANO ROLL LIVE',(20,10,220,24),(10,8,350,24),fontsize=16.0)
    box('open','textbutton','Open editor',(20,60,110,26),(10,38,110,26));box('open-msg','message','openeditor',(20,100,90,22));link('open','open-msg');link('open-msg','defer')
    box('load','textbutton','Load selected clip',(140,60,150,26),(128,38,150,26));box('load-msg','message','loadclip',(140,100,70,22));link('load','load-msg');link('load-msg','defer')
    box('status','comment','Select a MIDI clip, then Open editor.',(20,150,600,46),(10,70,560,46),fontsize=12.0);link('api','status',1)
    box('hint','comment','Live plays the clip through its existing instrument. Apply sends edits back.',(20,200,610,30),(10,118,560,28),fontsize=11.0)
    return {'patcher':{'fileversion':1,'appversion':{'major':9,'minor':0,'revision':0,'architecture':'x64','modernui':1},'classnamespace':'box',
        'rect':[100,100,800,480],'openinpresentation':1,'default_fontsize':12.0,'devicewidth':580.0,'boxes':boxes,'lines':lines,
        'dependency_cache':[{'name':n,'type':'TEXT','implicit':1} for n in ['bridge-max.js','bridge-node.js','clip-protocol.js']]}}

def chunk(tag,payload): return tag+struct.pack('>I',8+len(payload))+payload

def device(data,name):
    content=json.dumps(data,ensure_ascii=True,indent=2).encode()+b'\n\0'
    filename=name.encode()+b'\0';filename+=b'\0'*((-len(filename))%4)
    fields=chunk(b'type',b'JSON')+chunk(b'fnam',filename)+chunk(b'sz32',struct.pack('>I',len(content)))+chunk(b'of32',struct.pack('>I',16))
    fields+=chunk(b'vers',struct.pack('>I',0))+chunk(b'flag',struct.pack('>I',17))+chunk(b'mdat',struct.pack('>I',int(time.time())+2082844800))
    archive=b'mx@c'+struct.pack('>III',16,0,16+len(content))+content+chunk(b'dlst',chunk(b'dire',fields))
    return b'ampf'+struct.pack('<I',4)+b'mmmm'+b'meta'+struct.pack('<II',4,4)+b'ptch'+struct.pack('<I',len(archive))+archive

def build(destination=ROOT):
    destination=Path(destination);destination.mkdir(parents=True,exist_ok=True)
    data=patch();(destination/'Piano Roll Live.maxpat').write_text(json.dumps(data,indent=2)+'\n')
    payload=device(data,'Piano Roll Live.amxd');(destination/'Piano Roll Live.amxd').write_bytes(payload)
    assert struct.unpack_from('<I',payload,28)[0]==len(payload)-32
    directory=32+struct.unpack_from('>I',payload,44)[0]
    assert payload[directory:directory+4]==b'dlst'
    parsed=json.loads(payload[48:directory].rstrip(b'\0\n'))
    assert parsed==data
    print(destination/'Piano Roll Live.amxd')
if __name__=='__main__':
    import sys
    build(sys.argv[1] if len(sys.argv)>1 else ROOT)
