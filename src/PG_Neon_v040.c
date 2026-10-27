/*
 * Polygun Arena 1.6100/b1180 - PG Neon 0.4.0 (strict head / ESP HP / 5 speed levels)
 * EXPERIMENTAL live IL2CPP aim/ESP
 * Reads actual AimAssistTarget objects at runtime and calls Unity
 * Transform.LookAt on a visible enemy's top target transform.
 * Does not spoof headshots, change packets, or modify server data.
 * No test on actual iPhone; class and method existence checked live.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <dispatch/dispatch.h>
#include <mach/mach.h>

/* Objective-C runtime and UIKit without iPhone SDK headers. */
typedef void *Obj, *Sel, *Cls;
typedef struct {double x,y;} Point;
typedef struct {double w,h;} Size;
typedef struct {Point origin; Size size;} Rect;
extern Cls objc_getClass(const char*);
extern Cls objc_allocateClassPair(Cls,const char*,size_t);
extern void objc_registerClassPair(Cls);
extern int class_addMethod(Cls,Sel,void*,const char*);
extern Sel sel_registerName(const char*);
extern void objc_msgSend(void);
static void layoutMenu(Obj w);
static void refreshMenu(void);
extern Obj CFStringCreateWithCString(const void*,const char*,uint32_t);
extern void CFRelease(const void*);
extern uint32_t _dyld_image_count(void);
extern const void *_dyld_get_image_header(uint32_t);
extern const char *_dyld_get_image_name(uint32_t);
static Sel S(const char *n){return sel_registerName(n);}
static Obj St(const char *n){return CFStringCreateWithCString(NULL,n,0x08000100);}
#define MSG0(o,n) ((Obj(*)(Obj,Sel))objc_msgSend)((Obj)(o),S(n))
#define MSG1(o,n,a) ((Obj(*)(Obj,Sel,Obj))objc_msgSend)((Obj)(o),S(n),(Obj)(a))
static void setRect(Obj o,Rect f){((void(*)(Obj,Sel,Rect))objc_msgSend)(o,S("setFrame:"),f);}
static Rect bounds(Obj o){return ((Rect(*)(Obj,Sel))objc_msgSend)(o,S("bounds"));}
static void flag(Obj o,const char*n,int b){((void(*)(Obj,Sel,unsigned char))objc_msgSend)(o,S(n),(unsigned char)b);}
static void number(Obj o,const char*n,double d){((void(*)(Obj,Sel,double))objc_msgSend)(o,S(n),d);}
static Obj col(double r,double g,double b,double a){return ((Obj(*)(Obj,Sel,double,double,double,double))objc_msgSend)(objc_getClass("UIColor"),S("colorWithRed:green:blue:alpha:"),r,g,b,a);}
static void caption(Obj label,const char*s){Obj t=St(s); if(!t)return; MSG1(label,"setText:",t);CFRelease(t);}
static Obj makeLabel(Obj window,Rect r,double fontsize){
    Obj o=((Obj(*)(Obj,Sel,Rect))objc_msgSend)(MSG0(objc_getClass("UILabel"),"alloc"),S("initWithFrame:"),r);
    MSG1(o,"setTextColor:",col(.25,1,.40,1));
    MSG1(o,"setFont:",((Obj(*)(Obj,Sel,double))objc_msgSend)(objc_getClass("UIFont"),S("boldSystemFontOfSize:"),fontsize));
    flag(o,"setUserInteractionEnabled:",0);
    MSG1(window,"addSubview:",o);
    return o;
}
static Obj makeButton(Obj window,const char*txt,Rect r,Obj owner,const char*callback){
    Obj o=((Obj(*)(Obj,Sel,long))objc_msgSend)(objc_getClass("UIButton"),S("buttonWithType:"),0);
    MSG0(o,"retain"); /* Keep button alive across Unity window changes. */
    setRect(o,r);
    Obj txtObj=St(txt);
    ((void(*)(Obj,Sel,Obj,long))objc_msgSend)(o,S("setTitle:forState:"),txtObj,0);
    CFRelease(txtObj);
    MSG1(o,"setBackgroundColor:",col(.06,.11,.16,.87));
    ((void(*)(Obj,Sel,Obj,long))objc_msgSend)(o,S("setTitleColor:forState:"),col(.20,1,.5,1),0);
    ((void(*)(Obj,Sel,Obj,Sel,unsigned long))objc_msgSend)(o,S("addTarget:action:forControlEvents:"),owner,S(callback),64UL);
    number(MSG0(o,"layer"),"setCornerRadius:",10);
    MSG1(window,"addSubview:",o);
    return o;
}

/* IL2CPP C API loaded at runtime from the game's UnityFramework binary. */
typedef struct {float x,y,z;} Vec3;
typedef struct {void*klass,*monitor,*bounds; uintptr_t length; void*items[];} ILArray;
typedef struct {
    void* (*domain_get)(void);
    const void** (*domain_get_assemblies)(const void*,size_t*);
    const void* (*assembly_get_image)(const void*);
    const char* (*image_get_name)(const void*);
    void* (*class_from_name)(const void*,const char*,const char*);
    void* (*class_get_method_from_name)(void*,const char*,int);
    void* (*class_get_field_from_name)(void*,const char*);
    void* (*class_get_type)(void*);
    void* (*type_get_object)(void*);
    void* (*object_get_class)(void*);
    void* (*runtime_invoke)(void*,void*,void**,void**);
    void* (*object_unbox)(void*);
    void (*field_get_value)(void*,void*,void*);
    void (*field_static_get_value)(void*,void*);
    void* (*thread_attach)(void*);
    size_t (*array_length)(void*);
    unsigned int (*method_get_param_count)(void*);
    void* (*class_get_methods)(void*,void**);
    const char* (*method_get_name)(void*);
    const void* (*method_get_param)(void*,uint32_t);
    char* (*type_get_name)(const void*);
    void (*il_free)(void*);
} ILAPI;
static ILAPI a;
static void *clTarget,*clAssist,*clCamera,*clTransform,*clScreen,*clUnityObject,*clFPSAA,*clTime,*clFPSController,*clStats;
static void *mGetEnemy,*mGetDead,*mGetVisible,*mGetHeadFlag,*mGetFallback;
static void *mFindObjects,*mMainCamera,*mTransform,*mWorldToScreen,*mPosition,*mLookAt,*mScreenW,*mScreenH;
static void *mAutoFire,*mSetTimeScale,*mGetHealth,*mGetMaxHealth,*mGetInvulnerability;
static void *mFPSGetStats,*mFPSIsInitialized,*mStatsGetHealth,*mStatsSetHealth,*mStatsGetMaxHealth,*mStatsSetDamage;
static int sRuntimeReady=0;
static void *findImage(const char*name){
    void* dom=a.domain_get(); if(!dom)return NULL;
    size_t n=0;const void**assemblies=a.domain_get_assemblies(dom,&n);
    if(!assemblies || n>1000)return NULL;
    for(size_t i=0;i<n;i++){
        const void* im=a.assembly_get_image(assemblies[i]);
        const char* nm=im?a.image_get_name(im):NULL;
        if(nm && !strcmp(nm,name)) return (void*)im;
    }
    return NULL;
}
static void *invoke(void*m,void*obj,void**args){
    if(!m)return NULL;
    void*exc=NULL;
    void*r=a.runtime_invoke(m,obj,args,&exc);
    return exc?NULL:r;
}
static int unboxBool(void*p){return p && a.object_unbox(p) && *(uint8_t*)a.object_unbox(p)!=0;}
static int unboxInt(void*p){return p && a.object_unbox(p)? *(int*)a.object_unbox(p):0;}
static float unboxFloat(void*p){return p && a.object_unbox(p)? *(float*)a.object_unbox(p):-1.0f;}
static int unboxVec(void*p,Vec3*out){
    if(!p||!out)return 0;
    void* raw=a.object_unbox(p);if(!raw)return 0;
    memcpy(out,raw,sizeof(Vec3));return isfinite(out->x)&&isfinite(out->y)&&isfinite(out->z);
}
#define LOAD(name) do {a.name=(void*)dlsym(RTLD_DEFAULT,"il2cpp_" #name); if(!a.name)return 0;} while(0)
static void *findVector3LookAt(void*transformClass){
    if(!transformClass)return NULL;
    void*iter=NULL;void*method=NULL;
    int scan=0;
    while((method=a.class_get_methods(transformClass,&iter))!=NULL && scan++<512){
        const char*n=a.method_get_name(method);
        if(!n||strcmp(n,"LookAt")||a.method_get_param_count(method)!=1)continue;
        const void*param=a.method_get_param(method,0);
        char*typeName=param?a.type_get_name(param):NULL;
        int valid=typeName&&strstr(typeName,"UnityEngine.Vector3")!=NULL;
        if(typeName)a.il_free(typeName);
        if(valid)return method;
    }
    return NULL;
}
static int initIL(void){
    if(sRuntimeReady)return 1;
    LOAD(domain_get);LOAD(domain_get_assemblies);LOAD(assembly_get_image);
    LOAD(image_get_name);LOAD(class_from_name);LOAD(class_get_method_from_name);
    LOAD(class_get_field_from_name);LOAD(class_get_type);LOAD(type_get_object);
    LOAD(object_get_class);LOAD(runtime_invoke);LOAD(object_unbox);
    LOAD(field_get_value);LOAD(field_static_get_value);LOAD(thread_attach);
    LOAD(array_length);LOAD(method_get_param_count);
    LOAD(class_get_methods);LOAD(method_get_name);LOAD(method_get_param);
    LOAD(type_get_name);
    a.il_free=(void*)dlsym(RTLD_DEFAULT,"il2cpp_free");if(!a.il_free)return 0;
    void*dom=a.domain_get();if(!dom)return 0;
    a.thread_attach(dom);
    void*game=findImage("Assembly-CSharp.dll"),*core=findImage("UnityEngine.CoreModule.dll");
    if(!game||!core)return 0;
    clTarget=a.class_from_name(game,"","AimAssistTarget");
    clAssist=a.class_from_name(game,"CriticalStrikeInGame","AimAssistInGameSettings");
    clFPSAA=a.class_from_name(game,"CriticalStrikeFPSControlling","FPSControllerAimAssist");
    clCamera=a.class_from_name(core,"UnityEngine","Camera");
    clTransform=a.class_from_name(core,"UnityEngine","Transform");
    clScreen=a.class_from_name(core,"UnityEngine","Screen");
    clUnityObject=a.class_from_name(core,"UnityEngine","Object");
    clTime=a.class_from_name(core,"UnityEngine","Time");
    clFPSController=a.class_from_name(game,"CriticalStrikeFPSControlling","FPSControllerUltron");
    clStats=a.class_from_name(game,"","PlayerStats");
    void*component=a.class_from_name(core,"UnityEngine","Component");
    if(!clTarget||!clCamera||!clTransform||!clUnityObject||!component||!clScreen)return 0;
#define METHOD(out,cl,name,args) out=a.class_get_method_from_name(cl,name,args)
    METHOD(mGetEnemy,clTarget,"get_IsEnemyToLocalPlayer",0);
    METHOD(mGetDead,clTarget,"get_IsDead",0);
    METHOD(mGetVisible,clTarget,"get_IsVisible",0);
    METHOD(mGetHeadFlag,clTarget,"get_IsHeadShotAimAssistTarget",0);
    METHOD(mGetHealth,clTarget,"get_Health",0);
    METHOD(mGetMaxHealth,clTarget,"get_MaxHealth",0);
    METHOD(mGetInvulnerability,clTarget,"get_IsInvulnerable",0);
    METHOD(mGetFallback,clTarget,"get_TargetTransform",0);
    METHOD(mFindObjects,clUnityObject,"FindObjectsOfType",1);
    METHOD(mMainCamera,clCamera,"get_main",0);
    METHOD(mTransform,component,"get_transform",0);
    METHOD(mWorldToScreen,clCamera,"WorldToScreenPoint",1);
    METHOD(mPosition,clTransform,"get_position",0);
    mLookAt=findVector3LookAt(clTransform);
    METHOD(mScreenW,clScreen,"get_width",0);
    METHOD(mScreenH,clScreen,"get_height",0);
    if(clFPSAA)METHOD(mAutoFire,clFPSAA,"AutoFireSystem",0);
    if(clTime)METHOD(mSetTimeScale,clTime,"set_timeScale",1);
    if(clFPSController){METHOD(mFPSGetStats,clFPSController,"get_Stats",0);METHOD(mFPSIsInitialized,clFPSController,"get_IsInitialized",0);}
    if(clStats){METHOD(mStatsGetHealth,clStats,"get_Health",0);METHOD(mStatsSetHealth,clStats,"set_Health",1);
        METHOD(mStatsGetMaxHealth,clStats,"get_MaxHealth",0);METHOD(mStatsSetDamage,clStats,"set_DamageModifier",1);}
    /* Fail closed if engine API not present rather than calling guessed RVA. */
    if(!mGetEnemy||!mGetDead||!mGetFallback||!mFindObjects||!mMainCamera||!mTransform||!mWorldToScreen||!mPosition||!mScreenW||!mScreenH)return 0;
    /* No guessed LookAt overload. AIM simply won't call into Unity unless Vector3 is resolved. */
    sRuntimeReady=1;return 1;
}
static Obj sOwner,sWindow,sStatus,sAimButton,sESPButton,sFireButton,sSpeedButton,sGodButton,sDamageButton;
static Obj sLabels[24],sBoxes[24],sLines[24],sDots[24],sHPBars[24],sCross[2];
static Obj sPGButton,sPanel,sTitle,sInfo,sHideButton,sVolumeSession;
static int sStart=0,sAIM=0,sESP=0,sFIRE=0,sBuildOK=0,sSignatureOK=0;
static int sMenuOpen=0,sSpeedStep=0,sDidObserve=0;
static int sGod=0,sDamageMode=0,sStatsStatus=0;
static const float sDamageMultipliers[5]={1,1.5f,2,3,5};
static void *sTrackedStats=NULL;
static float sOriginalDamage=1.0f;
static uint8_t sOriginalInvulnerable=0;
static int sOwnStateSaved=0,sDamageWasApplied=0,sGodFlagWritten=0;
static unsigned sHealthCorrections=0;
static float sLastVolume=-1.0f;
static double sLastVolumeEvent=-1.0;
static int sCanAim=0;
static const float sSpeedValues[5]={1.0f,2.0f,3.0f,4.0f,5.0f};
static uint64_t sTickCount=0;
static int verifyGame(void){
    Obj b=MSG0(objc_getClass("NSBundle"),"mainBundle");
    Obj id=MSG0(b,"bundleIdentifier");
    Obj v=MSG1(b,"objectForInfoDictionaryKey:",St("CFBundleShortVersionString"));
    Obj build=MSG1(b,"objectForInfoDictionaryKey:",St("CFBundleVersion"));
    if(!id||!v||!build)return 0;
    int ok=((unsigned char(*)(Obj,Sel,Obj))objc_msgSend)(id,S("isEqualToString:"),St("com.polygon.arena"));
    ok &= ((unsigned char(*)(Obj,Sel,Obj))objc_msgSend)(v,S("isEqualToString:"),St("1.6100"));
    ok &= ((unsigned char(*)(Obj,Sel,Obj))objc_msgSend)(build,S("isEqualToString:"),St("1180"));
    return ok;
}
static int verifyUnity(void){
    for(uint32_t i=0;i<_dyld_image_count();i++){
        const char* name=_dyld_get_image_name(i);
        const uint8_t*base=(const uint8_t*)_dyld_get_image_header(i);
        if(!name||!base||!strstr(name,"UnityFramework.framework/UnityFramework"))continue;
        if(*(const uint32_t*)base!=0xfeedfacf)return 0;
        static const struct {uintptr_t rva;uint32_t code;} tests[]={
            {0x442BDF0,0xD10283FF},{0x442AF14,0xD10403FF},{0x439C168,0x39420000},
            {0x43A64E0,0xBD023400},{0x43A5BF0,0xBD018800},{0x439C450,0xF9401408}
        };
        for(int j=0;j<3;j++){
            uint32_t inst=0; vm_size_t copied=0;
            if(vm_read_overwrite(mach_task_self(),(vm_address_t)(uintptr_t)(base+tests[j].rva),4,(vm_address_t)(uintptr_t)&inst,&copied)!=KERN_SUCCESS || copied!=4)return 0;
            if(inst!=tests[j].code)return 0;
        }
        return 1;
    }
    return 0;
}
static void hideLabels(void){
    for(int i=0;i<24;i++){
        if(sLabels[i])flag(sLabels[i],"setHidden:",1);
        if(sBoxes[i])flag(sBoxes[i],"setHidden:",1);
        if(sHPBars[i])flag(sHPBars[i],"setHidden:",1);
        if(sLines[i])flag(sLines[i],"setHidden:",1);
        if(sDots[i])flag(sDots[i],"setHidden:",1);
    }
    for(int i=0;i<2;i++)if(sCross[i])flag(sCross[i],"setHidden:",1);
}
static void placeLine(Obj line,double x1,double y1,double x2,double y2,int visible){
    if(!line)return;
    double dx=x2-x1,dy=y2-y1,len=sqrt(dx*dx+dy*dy);
    if(!isfinite(len)||len<3||len>3000)return;
    double angle=atan2(dy,dx),co=cos(angle),si=sin(angle);
    typedef struct {double a,b,c,d,tx,ty;} Transform2D;
    Transform2D identity={1,0,0,1,0,0};
    ((void(*)(Obj,Sel,Transform2D))objc_msgSend)(line,S("setTransform:"),identity);
    setRect(line,(Rect){{0,0},{len,1.5}});
    Point middle={(x1+x2)*.5,(y1+y2)*.5};
    ((void(*)(Obj,Sel,Point))objc_msgSend)(line,S("setCenter:"),middle);
    Transform2D rot={co,si,-si,co,0,0};
    ((void(*)(Obj,Sel,Transform2D))objc_msgSend)(line,S("setTransform:"),rot);
    MSG1(line,"setBackgroundColor:", visible?col(.25,1,.70,.75):col(1,.66,.24,.50));
    flag(line,"setHidden:",0);
}
static void setTimeScale(float speed){
    if(!sRuntimeReady||!mSetTimeScale||!sBuildOK||!sSignatureOK)return;
    if(a.method_get_param_count(mSetTimeScale)!=1)return;
    void*args[]={&speed};invoke(mSetTimeScale,NULL,args);
}
static int getPos(void*transform,Vec3*out){return transform&&unboxVec(invoke(mPosition,transform,NULL),out);}
static void*getFieldObj(void*obj,const char*field){
    if(!obj)return NULL;
    void*fd=a.class_get_field_from_name(a.object_get_class(obj),field);
    void*ret=NULL;
    if(fd)a.field_get_value(obj,fd,&ret);
    return ret;
}
static void*headTransform(void*target){
    void*head=getFieldObj(target,"TopTransform");
    if(head)return head;
    return invoke(mGetFallback,target,NULL);
}
static void*findInstances(void*klass){
    if(!klass||!mFindObjects)return NULL;
    void*type=a.type_get_object(a.class_get_type(klass));
    if(!type)return NULL;
    void*argv[]={type};
    return invoke(mFindObjects,NULL,argv);
}
/* Primary target collection: game's current match manager; fallback to scene objects. */
static void*getTargetList(int*count,int*listType){
    *count=0;*listType=0;
    if(clAssist){
        void*staticField=a.class_get_field_from_name(clAssist,"<Instance>k__BackingField");
        void*manager=NULL;
        if(staticField)a.field_static_get_value(staticField,&manager);
        if(manager){
            void*container=getFieldObj(manager,"AimAssistTargets");
            if(container){
                void*type=a.object_get_class(container);
                void*getCount=a.class_get_method_from_name(type,"get_Count",0);
                void*getItem=a.class_get_method_from_name(type,"get_Item",1);
                if(getCount&&getItem){
                    int n=unboxInt(invoke(getCount,container,NULL));
                    if(n>0&&n<1000){*count=n;*listType=1;return container;}
                }
            }
        }
    }
    ILArray*array=(ILArray*)findInstances(clTarget);
    if(!array)return NULL;
    size_t n=a.array_length(array);
    if(n>1000)return NULL;
    *count=(int)n;*listType=2;return array;
}
static void*itemAt(void*obj,int i,int listType){
    if(listType==2){ILArray*arr=(ILArray*)obj;return arr->items[i];}
    if(listType==1){
        void*method=a.class_get_method_from_name(a.object_get_class(obj),"get_Item",1);
        void*args[]={&i};return invoke(method,obj,args);
    }
    return NULL;
}

/* Local-only HP and damage trainer. NEVER claim server authority.
 * Fail-closed: exactly one initialized FPS controller and signatures verified.
 * Do not preserve managed pointers between sessions; re-resolve each tick.
 */
static void* localPlayerStats(void){
    if(!clFPSController||!mFPSGetStats||!mStatsGetHealth||!mStatsGetMaxHealth)return NULL;
    ILArray*instances=(ILArray*)findInstances(clFPSController);
    if(!instances)return NULL;
    size_t n=a.array_length(instances);
    if(n!=1)return NULL; /* Do not accidentally alter another player. */
    void*control=instances->items[0];if(!control)return NULL;
    if(mFPSIsInitialized&&!unboxBool(invoke(mFPSIsInitialized,control,NULL)))return NULL;
    return invoke(mFPSGetStats,control,NULL);
}
static void restoreStats(void*stats){
    if(!stats||!sOwnStateSaved||stats!=sTrackedStats)return;
    if(sDamageWasApplied && mStatsSetDamage && a.method_get_param_count(mStatsSetDamage)==1){
        float value=sOriginalDamage;void*argv[]={&value};invoke(mStatsSetDamage,stats,argv);
    }
    /* Only revert the flag if this tweak forced it; never touch another Stats object. */
    if(sGodFlagWritten && *(uint8_t*)((uint8_t*)stats+0x221)==1)
        *(uint8_t*)((uint8_t*)stats+0x221)=sOriginalInvulnerable;
    sDamageWasApplied=0;sGodFlagWritten=0;sOwnStateSaved=0;sTrackedStats=NULL;
}
static int safeStatsAPIs(void){
    return sRuntimeReady&&sBuildOK&&sSignatureOK&&clStats&&mFPSGetStats&&
      mStatsGetHealth&&mStatsGetMaxHealth&&mStatsSetHealth&&mStatsSetDamage&&
      a.method_get_param_count(mStatsSetHealth)==1&&a.method_get_param_count(mStatsSetDamage)==1;
}
static void applyLocalStats(void){
    if(!safeStatsAPIs()){sStatsStatus=0;return;}
    void*stats=localPlayerStats();
    if(!stats){sStatsStatus=0;return;}
    if(sTrackedStats&&sTrackedStats!=stats){
        /* Previous instance may have been freed by IL2CPP: do NOT dereference. */
        sOwnStateSaved=0;sTrackedStats=NULL;sDamageWasApplied=0;sGodFlagWritten=0;
    }
    if(!sGod&&!sDamageMode){
        restoreStats(stats);sStatsStatus=1;return;
    }
    if(!sOwnStateSaved){
        memcpy(&sOriginalDamage,(uint8_t*)stats+0x188,sizeof(float));
        if(!isfinite(sOriginalDamage)||sOriginalDamage<.01f||sOriginalDamage>100.0f){sStatsStatus=-1;return;}
        sOriginalInvulnerable=*(uint8_t*)((uint8_t*)stats+0x221);
        sTrackedStats=stats;sOwnStateSaved=1;
    }
    sStatsStatus=2;
    if(sDamageMode && mStatsSetDamage){
        float buff=sOriginalDamage*sDamageMultipliers[sDamageMode];
        if(isfinite(buff)&&buff>0&&buff<=500.0f){
            void*args[]={&buff};invoke(mStatsSetDamage,stats,args);
            float observed=0;memcpy(&observed,(uint8_t*)stats+0x188,sizeof(float));
            if(!isfinite(observed)||fabs(observed-buff)>.0001f)sStatsStatus=-2;
            else sDamageWasApplied=1;
        }
    }else if(sDamageWasApplied && mStatsSetDamage){
        float base=sOriginalDamage;void*args[]={&base};invoke(mStatsSetDamage,stats,args);
        sDamageWasApplied=0;
    }
    if(sGod){
        float hp=unboxFloat(invoke(mStatsGetHealth,stats,NULL));
        float cap=unboxFloat(invoke(mStatsGetMaxHealth,stats,NULL));
        if(!isfinite(hp)||!isfinite(cap)||cap<=0||cap>1000000||hp<=0){
            sStatsStatus=-3;return;
        }
        if(hp<cap-.2f){
            sHealthCorrections++;
            void*args[]={&cap};invoke(mStatsSetHealth,stats,args);
        }
        *(uint8_t*)((uint8_t*)stats+0x221)=1; /* local invulnerability flag; not server godmode */
        sGodFlagWritten=1;
        float reread=unboxFloat(invoke(mStatsGetHealth,stats,NULL));
        if(!isfinite(reread)||reread<cap-1.0f)sStatsStatus=-4;
    }else{
        /* Don't overwrite a legitimate in-game temporary immunity if the flag wasn't forced. */
        if(sGodFlagWritten&&*(uint8_t*)((uint8_t*)stats+0x221)==1)
           *(uint8_t*)((uint8_t*)stats+0x221)=sOriginalInvulnerable;
        sGodFlagWritten=0;
    }
}

static void renderAimESP(void){
    if(!sBuildOK||!sSignatureOK)return;
    if(!initIL()){
        if(sStatus)caption(sStatus,"Unity IL2CPP chua san sang");
        return;
    }
    if(sSpeedStep>0 && (sTickCount%10)==0)setTimeScale(sSpeedValues[sSpeedStep]);
    if(sGod||sDamageMode||sOwnStateSaved)applyLocalStats();
    if(!sAIM&&!sESP&&!sFIRE){hideLabels();return;}
    void*camera=invoke(mMainCamera,NULL,NULL);
    int px=unboxInt(invoke(mScreenW,NULL,NULL)),py=unboxInt(invoke(mScreenH,NULL,NULL));
    if(!camera||px<=0||py<=0)return;
    Rect area=bounds(sWindow);
    if(area.size.w<=0||area.size.h<=0)return;
    int total=0,collection=0;void*targets=getTargetList(&total,&collection);
    hideLabels();
    if(sESP){
        if(sCross[0]){setRect(sCross[0],(Rect){{area.size.w/2-8,area.size.h/2-0.75},{16,1.5}});flag(sCross[0],"setHidden:",0);}
        if(sCross[1]){setRect(sCross[1],(Rect){{area.size.w/2-0.75,area.size.h/2-8},{1.5,16}});flag(sCross[1],"setHidden:",0);}
    }
    if(!targets)return;
    int shown=0,enemies=0;
    void*best=NULL;Vec3 bestPos={0};double bestDist=1e15;
    for(int i=0;i<total&&i<128;i++){
        void*target=itemAt(targets,i,collection);
        if(!target||!unboxBool(invoke(mGetEnemy,target,NULL))||unboxBool(invoke(mGetDead,target,NULL)))continue;
        enemies++;
        void*transform=headTransform(target);Vec3 top={0};
        if(!getPos(transform,&top))continue;
        void*args[]={&top};Vec3 screen={0};
        if(!unboxVec(invoke(mWorldToScreen,camera,args),&screen)||screen.z<=0)continue;
        double x=screen.x/px*area.size.w,y=(1-screen.y/(double)py)*area.size.h;
        if(!isfinite(x)||!isfinite(y))continue;
        double dx=x-area.size.w/2,dy=y-area.size.h/2,d=dx*dx+dy*dy;
        int visible=!mGetVisible||unboxBool(invoke(mGetVisible,target,NULL));
        int invuln=mGetInvulnerability && unboxBool(invoke(mGetInvulnerability,target,NULL));
        int headFlag=mGetHeadFlag && unboxBool(invoke(mGetHeadFlag,target,NULL));
        double score=d;
        /* HEAD ONLY: never aim at fallback body transforms or temporarily invulnerable targets. */
        if(headFlag && visible && !invuln && score<bestDist && d<240*240){bestDist=score;best=target;bestPos=top;}
        if(sESP&&shown<24&&x>=-50&&x<=area.size.w+50&&y>=-50&&y<=area.size.h+50){
            int index=shown++;
            char buf[100];
            float hp=mGetHealth?unboxFloat(invoke(mGetHealth,target,NULL)):-1;
            float maxhp=mGetMaxHealth?unboxFloat(invoke(mGetMaxHealth,target,NULL)):-1;
            int hpOK=isfinite(hp)&&isfinite(maxhp)&&maxhp>0&&maxhp<1000000&&hp>=0&&hp<=maxhp*1.5f;
            if(hpOK)snprintf(buf,sizeof buf,"%s  HP %.0f/%.0f%s",visible?"ENEMY":"HIDDEN",(double)hp,(double)maxhp,invuln?"  INV":"");
            else snprintf(buf,sizeof buf,"%s %d%s",visible?"ENEMY":"HIDDEN",i+1,invuln?" INV":"");
            Obj label=sLabels[index];
            if(label){caption(label,buf);setRect(label,(Rect){{x-58,y-33},{145,20}});flag(label,"setHidden:",0);}
            if(sDots[index]){setRect(sDots[index],(Rect){{x-3,y-3},{6,6}});flag(sDots[index],"setHidden:",0);}
            placeLine(sLines[index],area.size.w/2,area.size.h*.02,x,y,visible);
            if(hpOK && sHPBars[index]){
                double ratio=fmax(0,fmin(1,(double)hp/maxhp));
                setRect(sHPBars[index],(Rect){{x-24,y-12},{48*ratio,3}});
                MSG1(sHPBars[index],"setBackgroundColor:",col(1.0-ratio,.35+ratio*.65,.18,.94));
                flag(sHPBars[index],"setHidden:",0);
            }
            void*bottom=getFieldObj(target,"BottomTransform");Vec3 bpos,bscreen;
            if(bottom&&getPos(bottom,&bpos)){
                void*ap[]={&bpos};
                if(unboxVec(invoke(mWorldToScreen,camera,ap),&bscreen)&&bscreen.z>0){
                    double by=(1-bscreen.y/(double)py)*area.size.h;
                    double h=fabs(by-y);
                    if(isfinite(h)&&h>5&&h<area.size.h*2){
                        double bw=h*.45;Obj box=sBoxes[index];
                        if(box){
                            setRect(box,(Rect){{x-bw/2,fmin(y,by)},{bw,h}});
                            MSG1(MSG0(box,"layer"),"setBorderColor:",MSG0(col(visible?.20:1,visible?1:.60,.6,.85),"CGColor"));
                            flag(box,"setHidden:",0);
                        }
                    }
                }
            }
        }
    }
    /* Risk guard: gameplay invocations run less often than ESP rendering.
     * Disable unsupported or invalid method calls instead of guessing RVAs.
     * Unity object lifetime is not guaranteed; behavior still requires device testing.
     */
    if(sAIM&&best&&mLookAt&&(sTickCount%4)==0){
        void* camTransform=invoke(mTransform,camera,NULL);
        if(camTransform&&mLookAt&&a.method_get_param_count(mLookAt)==1){
            void*params[]={&bestPos};invoke(mLookAt,camTransform,params);
            sCanAim=1;
        }
    }
    if(sFIRE&&best&&clFPSAA&&mAutoFire&&a.method_get_param_count(mAutoFire)==0){
        ILArray*aa=(ILArray*)findInstances(clFPSAA);
        if(aa&&a.array_length(aa)>0&&aa->items[0])invoke(mAutoFire,aa->items[0],NULL);
    }
    if(sStatus){
        char stat[160];snprintf(stat,sizeof stat,"PG 0.4.0 | ENEMY %d | ESP %s | HEAD %s | FIRE %s",enemies,sESP?"ON":"OFF",sAIM?"ON":"OFF",sFIRE?"ON":"OFF");
        caption(sStatus,stat);
    }
}
/* UI recovery is independent of game/runtime validation. */
static long integer(Obj o,const char*n){return ((long(*)(Obj,Sel))objc_msgSend)(o,S(n));}
static int boolean(Obj o,const char*n){return ((unsigned char(*)(Obj,Sel))objc_msgSend)(o,S(n))!=0;}
static Obj at(Obj a,unsigned long i){return ((Obj(*)(Obj,Sel,unsigned long))objc_msgSend)(a,S("objectAtIndex:"),i);}
static int isTarget(void){
    Obj bid=MSG0(MSG0(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
    Obj expect=St("com.polygon.arena");
    int ok=((unsigned char(*)(Obj,Sel,Obj))objc_msgSend)(bid,S("isEqualToString:"),expect);
    CFRelease(expect);return ok;
}
static Obj chooseWindow(Obj windows){
    unsigned long n=(unsigned long)integer(windows,"count");
    if(n>128)return NULL;
    Obj best=NULL;double largest=0;
    for(unsigned long i=0;i<n;i++){
        Obj w=at(windows,i);
        if(boolean(w,"isHidden"))continue;
        double level=((double(*)(Obj,Sel))objc_msgSend)(w,S("windowLevel"));
        Rect b=bounds(w);if(fabs(level)>.01||b.size.w<100||b.size.h<100)continue;
        if(boolean(w,"isKeyWindow"))return w;
        double area=b.size.w*b.size.h;if(area>largest){largest=area;best=w;}
    }
    return best;
}
static Obj gameWindow(void){
    Obj app=MSG0(objc_getClass("UIApplication"),"sharedApplication");
    Obj scenes=MSG0(MSG0(app,"connectedScenes"),"allObjects");
    unsigned long n=(unsigned long)integer(scenes,"count");
    if(n<=64)for(unsigned long i=0;i<n;i++){
        Obj sc=at(scenes,i);
        int kind=((unsigned char(*)(Obj,Sel,Cls))objc_msgSend)(sc,S("isKindOfClass:"),objc_getClass("UIWindowScene"));
        if(!kind||integer(sc,"activationState")!=0)continue;
        Obj w=chooseWindow(MSG0(sc,"windows"));if(w)return w;
    }
    return chooseWindow(MSG0(app,"windows"));
}
static void title(Obj b,const char*n,int enabled){
    if(!b)return;
    char txt[44];snprintf(txt,sizeof txt,"%s  %s",n,enabled?"ON":"OFF");
    Obj t=St(txt);((void(*)(Obj,Sel,Obj,long))objc_msgSend)(b,S("setTitle:forState:"),t,0);CFRelease(t);
    MSG1(b,"setBackgroundColor:",enabled?col(.04,.34,.30,.96):col(.09,.13,.22,.98));
    ((void(*)(Obj,Sel,Obj,long))objc_msgSend)(b,S("setTitleColor:forState:"),enabled?col(.42,1,.78,1):col(.8,.91,1,1),0);
}
static void mount(Obj w,Obj v){
    if(!v)return;
    if(MSG0(v,"superview")!=w)MSG1(w,"addSubview:",v);
    MSG1(w,"bringSubviewToFront:",v);
}
static Obj makeView(Obj parent,Rect r,double red,double green,double blue,double alpha){
    Obj o=((Obj(*)(Obj,Sel,Rect))objc_msgSend)(MSG0(objc_getClass("UIView"),"alloc"),S("initWithFrame:"),r);
    MSG1(o,"setBackgroundColor:",col(red,green,blue,alpha));
    MSG1(parent,"addSubview:",o);
    return o;
}
static void pgStyle(Obj v,double radius,double border){
    Obj layer=MSG0(v,"layer");number(layer,"setCornerRadius:",radius);
    number(layer,"setBorderWidth:",border);
    MSG1(layer,"setBorderColor:",MSG0(col(.17,.95,.74,.75),"CGColor"));
    flag(v,"setClipsToBounds:",1);
}
static void refreshMenu(void){
    if(!sStart)return;
    title(sAimButton,"AIM",sAIM);title(sESPButton,"ESP",sESP);title(sFireButton,"FIRE",sFIRE);
    title(sGodButton,"GOD LOCAL",sGod);
    if(sSpeedButton){
        char t[36];snprintf(t,sizeof t,"SPEED  x%.1f",(double)sSpeedValues[sSpeedStep]);
        Obj v=St(t);((void(*)(Obj,Sel,Obj,long))objc_msgSend)(sSpeedButton,S("setTitle:forState:"),v,0);CFRelease(v);
        MSG1(sSpeedButton,"setBackgroundColor:",sSpeedStep>0?col(.04,.34,.30,.96):col(.09,.13,.22,.98));
    }
    if(sDamageButton){
        char t[40];snprintf(t,sizeof t,"DMG x%.1f (local)",(double)sDamageMultipliers[sDamageMode]);
        Obj v=St(t);((void(*)(Obj,Sel,Obj,long))objc_msgSend)(sDamageButton,S("setTitle:forState:"),v,0);CFRelease(v);
        MSG1(sDamageButton,"setBackgroundColor:",sDamageMode?col(.04,.34,.30,.96):col(.09,.13,.22,.98));
    }
    if(sPGButton)MSG1(sPGButton,"setBackgroundColor:",sMenuOpen?col(.04,.35,.29,.96):col(.04,.09,.19,.94));
    if(sPanel)flag(sPanel,"setHidden:",!sMenuOpen);
}
static void layoutMenu(Obj w){
    if(!sStart||!w)return;
    int windowChanged=(w!=sWindow);
    if(windowChanged){MSG0(w,"retain");Obj old=sWindow;sWindow=w;if(old)MSG0(old,"release");}
    typedef struct {double top,left,bottom,right;} Insets;
    Insets safe=((Insets(*)(Obj,Sel))objc_msgSend)(w,S("safeAreaInsets"));
    Rect b=bounds(w);double x=safe.left+10,y=safe.top+58;
    /* Only PG button is visible while the settings panel is collapsed. */
    setRect(sPGButton,(Rect){{b.size.w-safe.right-50,safe.top+6},{42,42}});
    double pw=fmin(310,fmax(240,b.size.w-safe.left-safe.right-20));
    double panelX=x,panelY=y+49;
    if(panelY+370>b.size.h-safe.bottom-6)panelY=fmax(safe.top+8,y-372);
    setRect(sPanel,(Rect){{panelX,panelY},{pw,360}});
    setRect(sStatus,(Rect){{12,54},{pw-24,30}});
    setRect(sTitle,(Rect){{14,12},{pw-28,32}});
    setRect(sInfo,(Rect){{14,316},{pw-28,34}});
    double buttonWidth=(pw-38)/2;
    setRect(sAimButton,(Rect){{12,94},{buttonWidth,38}});
    setRect(sESPButton,(Rect){{26+buttonWidth,94},{buttonWidth,38}});
    setRect(sFireButton,(Rect){{12,140},{buttonWidth,38}});
    setRect(sSpeedButton,(Rect){{26+buttonWidth,140},{buttonWidth,38}});
    setRect(sGodButton,(Rect){{12,187},{buttonWidth,38}});
    setRect(sDamageButton,(Rect){{26+buttonWidth,187},{buttonWidth,38}});
    setRect(sHideButton,(Rect){{12,244},{pw-24,38}});
    if(windowChanged){
        for(int i=0;i<24;i++){
            mount(w,sLines[i]);mount(w,sBoxes[i]);mount(w,sDots[i]);mount(w,sHPBars[i]);mount(w,sLabels[i]);
        }
        for(int i=0;i<2;i++)mount(w,sCross[i]);
    }
    mount(w,sPanel);mount(w,sPGButton);
    refreshMenu();
}
static void onActive(Obj self,Sel sel,Obj note);
static void onTick(Obj self,Sel sel,Obj timer){
    (void)self;(void)sel;(void)timer;sTickCount++;
    Obj app=MSG0(objc_getClass("UIApplication"),"sharedApplication");
    if(integer(app,"applicationState")!=0){
        sGod=0;sDamageMode=0; /* Restore when gameplay becomes active again. */
        return;
    }
    if(!sStart){onActive(NULL,NULL,NULL);return;}
    if((sTickCount%10)==0){Obj w=gameWindow();if(w)layoutMenu(w);}
    if(!sBuildOK){caption(sStatus,"Khac phien ban 1.6100/1180");hideLabels();return;}
    if(!sAIM&&!sESP&&!sFIRE&&sSpeedStep==0&&!sGod&&!sDamageMode&&!sOwnStateSaved){caption(sStatus,"PG 0.4.0 DA NAP | READY");hideLabels();return;}
    if(!sSignatureOK||(sTickCount%10)==0)sSignatureOK=verifyUnity();
    if(!sSignatureOK){caption(sStatus,"Unity chua san sang / sai chu ky");hideLabels();return;}
    if((sTickCount%2)==0)renderAimESP();
    if(sInfo && (sTickCount%4)==0){
        const char *state=sStatsStatus==2?"LOCAL APPLIED":(sStatsStatus==1?"READY":
        (sStatsStatus==0?"NO LOCAL STATS": "CHECK FAILED"));
        char info[145];snprintf(info,sizeof(info),"HP/DMG: %s | KHONG XAC NHAN SERVER",state);
        caption(sInfo,info);
    }
}
static void toggleESP(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(sBuildOK)sESP=!sESP;refreshMenu();if(!sESP)hideLabels();
}
static void toggleAIM(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(sBuildOK)sAIM=!sAIM;refreshMenu();
}
static void toggleFIRE(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(sBuildOK)sFIRE=!sFIRE;refreshMenu();
}
static void toggleGod(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(!sBuildOK)return;sGod=!sGod;refreshMenu();
}
static void toggleDamage(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(!sBuildOK)return;sDamageMode=(sDamageMode+1)%5;refreshMenu();
}
static void toggleMenu(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    sMenuOpen=!sMenuOpen;refreshMenu();
}
static void toggleSpeed(Obj self,Sel sel,Obj sender){(void)self;(void)sel;(void)sender;
    if(!sBuildOK)return;
    int old=sSpeedStep;sSpeedStep=(sSpeedStep+1)%5;
    if(old>0&&sSpeedStep==0)setTimeScale(1.0f);
    refreshMenu();
}
/* AVAudioSession KVO + volume notification as a fallback. No private button-hook patching. */
static double nowSec(void){return ((double(*)(Obj,Sel))objc_msgSend)(objc_getClass("NSDate"),S("timeIntervalSinceReferenceDate"));}
static float outputVolume(void){
    if(!sVolumeSession)return -1;
    return ((float(*)(Obj,Sel))objc_msgSend)(sVolumeSession,S("outputVolume"));
}
static void volumeRise(void){
    double t=nowSec();if(sLastVolumeEvent>=0&&t-sLastVolumeEvent<.25)return;
    sLastVolumeEvent=t;sMenuOpen=!sMenuOpen;refreshMenu();
}
/* AVAudioSession KVO may be delivered off the main thread. Never touch UIKit there. */
static void pgVolumeMain(void*context){
    (void)context;
    if(!sStart)return;
    float v=outputVolume();
    if(v>=0){
        if(sLastVolume>=0&&v>sLastVolume+0.001f)volumeRise();
        sLastVolume=v;
    }
}
static void pgObserveVolume(Obj self,Sel sel,Obj key,Obj object,Obj change,void*context){
    (void)self;(void)sel;(void)key;(void)object;(void)change;(void)context;
    dispatch_async_f(dispatch_get_main_queue(),NULL,pgVolumeMain);
}
static void pgVolumeNotification(Obj self,Sel sel,Obj note){
    (void)self;(void)sel;(void)note;
    dispatch_async_f(dispatch_get_main_queue(),NULL,pgVolumeMain);
}
static void setupVolume(void){
    if(sDidObserve)return;
    /* Unity doesn't always load AVFAudio itself. If needed, request the system framework. */
    Cls cls=objc_getClass("AVAudioSession");
    if(!cls){dlopen("/System/Library/Frameworks/AVFAudio.framework/AVFAudio",1);
             cls=objc_getClass("AVAudioSession");}
    if(cls){
        sVolumeSession=MSG0(cls,"sharedInstance");
        if(sVolumeSession){
            sLastVolume=outputVolume();
            Obj key=St("outputVolume");
            ((void(*)(Obj,Sel,Obj,Obj,unsigned long,void*))objc_msgSend)(sVolumeSession,S("addObserver:forKeyPath:options:context:"),sOwner,key,1UL,NULL);
            CFRelease(key);
        }
    }
    Obj nc=MSG0(objc_getClass("NSNotificationCenter"),"defaultCenter");
    Obj noti=St("AVSystemController_SystemVolumeDidChangeNotification");
    ((void(*)(Obj,Sel,Obj,Sel,Obj,Obj))objc_msgSend)(nc,S("addObserver:selector:name:object:"),sOwner,S("pgVolumeChanged:"),noti,NULL);
    CFRelease(noti);sDidObserve=1;
}
static void onActive(Obj self,Sel sel,Obj note){(void)self;(void)sel;(void)note;
    if(!isTarget())return;
    Obj window=gameWindow();if(!window)return;
    if(sStart){layoutMenu(window);return;}
    sBuildOK=verifyGame();sWindow=MSG0(window,"retain");
    sPGButton=makeButton(window,"PG",(Rect){{10,100},{42,42}},sOwner,"pgMenu:");
    pgStyle(sPGButton,21,1.2);
    MSG1(sPGButton,"setBackgroundColor:",col(.04,.09,.19,.96));
    sPanel=makeView(window,(Rect){{10,149},{310,360}},.025,.035,.08,.96);
    pgStyle(sPanel,16,1);
    sTitle=makeLabel(sPanel,(Rect){{14,12},{276,32}},20);
    caption(sTitle,"PG   NEON  0.4.0");
    sStatus=makeLabel(sPanel,(Rect){{12,54},{276,30}},11);
    caption(sStatus,"PG 0.4.0 DA NAP | READY");
    sAimButton=makeButton(sPanel,"AIM OFF",(Rect){{12,94},{134,38}},sOwner,"pgAim:");
    sESPButton=makeButton(sPanel,"ESP OFF",(Rect){{160,94},{134,38}},sOwner,"pgESP:");
    sFireButton=makeButton(sPanel,"FIRE OFF",(Rect){{12,140},{134,38}},sOwner,"pgFire:");
    sSpeedButton=makeButton(sPanel,"SPEED x1.0",(Rect){{160,140},{134,38}},sOwner,"pgSpeed:");
    sGodButton=makeButton(sPanel,"GOD LOCAL OFF",(Rect){{12,187},{134,38}},sOwner,"pgGod:");
    sDamageButton=makeButton(sPanel,"DMG x1.0",(Rect){{160,187},{134,38}},sOwner,"pgDamage:");
    sHideButton=makeButton(sPanel,"THU GON MENU",(Rect){{12,244},{282,38}},sOwner,"pgMenu:");
    sInfo=makeLabel(sPanel,(Rect){{14,316},{284,34}},10);
    caption(sInfo,"GOD/DMG LOCAL ONLY | CHUA XAC MINH SERVER");
    for(int i=0;i<24;i++){
        sLines[i]=makeView(window,(Rect){{0,0},{1,1.5}},.3,1,.74,.78);
        flag(sLines[i],"setUserInteractionEnabled:",0);flag(sLines[i],"setHidden:",1);
        sBoxes[i]=makeView(window,(Rect){{0,0},{1,1}},.08,.18,.24,.05);
        flag(sBoxes[i],"setUserInteractionEnabled:",0);
        Obj layer=MSG0(sBoxes[i],"layer");number(layer,"setBorderWidth:",1.35);
        MSG1(layer,"setBorderColor:",MSG0(col(.2,1,.7,.85),"CGColor"));
        flag(sBoxes[i],"setHidden:",1);
        sHPBars[i]=makeView(window,(Rect){{0,0},{48,3}},.16,1,.6,.95);
        flag(sHPBars[i],"setUserInteractionEnabled:",0);flag(sHPBars[i],"setHidden:",1);
        sDots[i]=makeView(window,(Rect){{0,0},{6,6}},.4,1,.8,.98);
        flag(sDots[i],"setUserInteractionEnabled:",0);
        number(MSG0(sDots[i],"layer"),"setCornerRadius:",3);
        flag(sDots[i],"setHidden:",1);
        sLabels[i]=makeLabel(window,(Rect){{0,0},{90,20}},10);
        flag(sLabels[i],"setHidden:",1);
    }
    for(int i=0;i<2;i++){
        sCross[i]=makeView(window,(Rect){{0,0},{1,1}},.37,1,.75,.95);
        flag(sCross[i],"setUserInteractionEnabled:",0);flag(sCross[i],"setHidden:",1);
    }
    sStart=1;layoutMenu(window);setupVolume();
}
static void boot(void*context){
    (void)context;if(!isTarget())return;
    Cls base=objc_getClass("NSObject");if(!base)return;
    Cls cls=objc_allocateClassPair(base,"PGNeonController030",0);
    if(!cls)return;
    class_addMethod(cls,S("pgActive:"),(void*)onActive,"v@:@");
    class_addMethod(cls,S("pgTick:"),(void*)onTick,"v@:@");
    class_addMethod(cls,S("pgAim:"),(void*)toggleAIM,"v@:@");
    class_addMethod(cls,S("pgESP:"),(void*)toggleESP,"v@:@");
    class_addMethod(cls,S("pgFire:"),(void*)toggleFIRE,"v@:@");
    class_addMethod(cls,S("pgMenu:"),(void*)toggleMenu,"v@:@");
    class_addMethod(cls,S("pgSpeed:"),(void*)toggleSpeed,"v@:@");
    class_addMethod(cls,S("pgGod:"),(void*)toggleGod,"v@:@");
    class_addMethod(cls,S("pgDamage:"),(void*)toggleDamage,"v@:@");
    class_addMethod(cls,S("pgVolumeChanged:"),(void*)pgVolumeNotification,"v@:@");
    class_addMethod(cls,S("observeValueForKeyPath:ofObject:change:context:"),(void*)pgObserveVolume,"v@:@@@^v");
    objc_registerClassPair(cls);sOwner=MSG0(MSG0(cls,"alloc"),"init");
    Obj nc=MSG0(objc_getClass("NSNotificationCenter"),"defaultCenter");
    const char*notes[]={"UIApplicationDidBecomeActiveNotification","UISceneDidActivateNotification","UIWindowDidBecomeVisibleNotification"};
    for(unsigned i=0;i<3;i++){
        Obj n=St(notes[i]);
        ((void(*)(Obj,Sel,Obj,Sel,Obj,Obj))objc_msgSend)(nc,S("addObserver:selector:name:object:"),sOwner,S("pgActive:"),n,NULL);
        CFRelease(n);
    }
    Obj timer=((Obj(*)(Obj,Sel,double,Obj,Sel,Obj,unsigned char))objc_msgSend)(objc_getClass("NSTimer"),S("timerWithTimeInterval:target:selector:userInfo:repeats:"),.1,sOwner,S("pgTick:"),NULL,1);
    extern Obj const NSRunLoopCommonModes;
    ((void(*)(Obj,Sel,Obj,Obj))objc_msgSend)(MSG0(objc_getClass("NSRunLoop"),"mainRunLoop"),S("addTimer:forMode:"),timer,NSRunLoopCommonModes);
    onActive(NULL,NULL,NULL);
}
__attribute__((constructor)) static void init(void){dispatch_async_f(dispatch_get_main_queue(),NULL,boot);}
