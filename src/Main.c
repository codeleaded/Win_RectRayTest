#include "/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "/home/codeleaded/System/Static/Library/Random.h"
#include "/home/codeleaded/System/Static/Library/TransformedView.h"


#define INTERSECTION_NONE	0
#define INTERSECTION_PX		1
#define INTERSECTION_NX		2
#define INTERSECTION_PY		3
#define INTERSECTION_NY		4

TransformedView tv;

Vec2 Direction_Get(char type){
	if(type==INTERSECTION_PX) return (Vec2){ 1.0f, 0.0f };
	if(type==INTERSECTION_NX) return (Vec2){-1.0f, 0.0f };
	if(type==INTERSECTION_PY) return (Vec2){ 0.0f, 1.0f };
	if(type==INTERSECTION_NY) return (Vec2){ 0.0f,-1.0f };
	return (Vec2){ 0.0f,0.0f };
}
char Rect_Ray_NearIntersection(Vec2 ray_origin,Vec2 ray_dir,Vec2 target_p,Vec2 target_l,Vec2* contact_point,float* t_hit_near){
	*contact_point = (Vec2){ 0.0f,0.0f };

	if(ray_dir.y==0.0f){
		if(ray_origin.y > target_p.y && ray_origin.y < target_p.y + target_l.y){
			if(ray_dir.x == 0.0f)
				return INTERSECTION_NONE;
			if(ray_dir.x > 0.0f){
				if(target_p.x > ray_origin.x && target_p.x < ray_origin.x + ray_dir.x){
					*contact_point = (Vec2){ target_p.x,ray_origin.y };
					return INTERSECTION_PX;
				}
			}else{
				if(target_p.x < ray_origin.x && target_p.x > ray_origin.x + ray_dir.x){
					*contact_point = (Vec2){ target_p.x + target_l.x,ray_origin.y };
					return INTERSECTION_NX;
				}
			}
		}else{
			return INTERSECTION_NONE;
		}
	}
	if(ray_dir.x==0.0f){
		if(ray_origin.x > target_p.x && ray_origin.x < target_p.x + target_l.x){
			if(ray_dir.y == 0.0f)
				return INTERSECTION_NONE;
			if(ray_dir.y > 0.0f){
				if(target_p.y > ray_origin.y && target_p.y < ray_origin.y + ray_dir.y){
					*contact_point = (Vec2){ ray_origin.x,target_p.y };
					return INTERSECTION_PY;
				}
			}else{
				if(target_p.y < ray_origin.y && target_p.y > ray_origin.y + ray_dir.y){
					*contact_point = (Vec2){ ray_origin.x,target_p.y + target_l.y };
					return INTERSECTION_NY;
				}
			}
		}else{
			return INTERSECTION_NONE;
		}
	}

	Vec2 invdir = { 1.0f / ray_dir.x,1.0f / ray_dir.y };
	Vec2 t_near = Vec2_Mul(Vec2_Sub(target_p,ray_origin),invdir);
	Vec2 t_far = Vec2_Mul(Vec2_Sub(Vec2_Add(target_p,target_l),ray_origin),invdir);

	if (t_near.x > t_far.x) F32_Swap(&t_near.x,&t_far.x);
	if (t_near.y > t_far.y) F32_Swap(&t_near.y,&t_far.y);
			
	if (t_near.x > t_far.y || t_near.y > t_far.x) return INTERSECTION_NONE;
	
	*t_hit_near = F32_Max(t_near.x, t_near.y);
	float t_hit_far = F32_Min(t_far.x, t_far.y);
	
	if (t_hit_far < 0.0f)
		return INTERSECTION_NONE;

	*contact_point = Vec2_Add(Vec2_Mulf(ray_dir,*t_hit_near),ray_origin);
	if (t_near.x > t_near.y)
		if (invdir.x < 0.0f)
			return INTERSECTION_PX;
		else
			return INTERSECTION_NX;
	else if (t_near.x < t_near.y)
		if (invdir.y < 0.0f)
			return INTERSECTION_PY;
		else
			return INTERSECTION_NY;
	
	return true;
}
char Rect_Rect_RI_Solver(Vec2 p1,Vec2* t1,Vec2 l1,Vec2 p2,Vec2 l2){
	const Vec2 p_m = Vec2_Add(p1,Vec2_Mulf(l1,0.5f));
	const Vec2 t_m = Vec2_Add(*t1,Vec2_Mulf(l1,0.5f));
	const Vec2 dir = Vec2_Sub(t_m,p_m);

	const Vec2 p_ex = Vec2_Sub(p2,Vec2_Mulf(l1,0.5f));
	const Vec2 l_ex = Vec2_Add(l1,l2);

	float t = 0.0f;
	Vec2 cp = { 0.0f,0.0f };
	const char type = Rect_Ray_NearIntersection(p_m,dir,p_ex,l_ex,&cp,&t);

	if(type != INTERSECTION_NONE && t>=0.0f && t<=1.0f){
		const Vec2 pS = TransformedView_WorldScreenPos(&tv,cp);
		const float l = TransformedView_WorldScreenLX(&tv,0.5f);
		RenderCircleWire(pS,l,WHITE,1.0f);
		
		const Vec2 n = Direction_Get(type);
		const Vec2 c_m = Vec2_Sub(cp,Vec2_Mulf(l1,0.5f));
		const Vec2 vel = { dir.x * F32_Abs(n.y) * (1.0f - t),dir.y * F32_Abs(n.x) * (1.0f - t) };
		*t1 = Vec2_Add(c_m,vel);
		return type;
	}
	return INTERSECTION_NONE;
}

Vec2* Focused = NULL;
TransformedView tv;

typedef struct SRect {
    Vec2 p;
    Vec2 l;
	Vec2 v;
	Vec2 a;
} SRect;

void SRect_Render(SRect* r){
    Vec2 p = TransformedView_WorldScreenPos(&tv,r->p);
	Vec2 l = TransformedView_WorldScreenLength(&tv,r->l);
	RenderRectWire(p.x,p.y,l.x,l.y,RED,1.0f);
}


#define SPEED	5.0f
#define SIZEX	16
#define SIZEY	16

char world[SIZEY * SIZEX];
SRect player;

int Rect_Compare(Rect* r1,Rect* r2){
	Vec2 m = Vec2_Add(player.p,Vec2_Mulf(player.l,0.5f));
	float d1 = Vec2_Mag(Vec2_Sub(Vec2_Add(r1->p,Vec2_Mulf(r1->d,0.5f)),m));
	float d2 = Vec2_Mag(Vec2_Sub(Vec2_Add(r2->p,Vec2_Mulf(r2->d,0.5f)),m));
	return d1 == d2 ? 0 : (d1 > d2 ? 1 : -1);
}

void Setup(AlxWindow* w){
	tv = TransformedView_New((Vec2){ GetWidth() * 0.02f,GetWidth() * 0.02f });
    TransformedView_Offset(&tv,(Vec2){ -1.0f,-1.0f });

	player = (SRect){ 0.0f,0.0f,0.5f,0.5f,0.0f,0.0f,0.0f,-10.0f };
	memset(world,0,sizeof(world));
}

void Update(AlxWindow* w){
    TransformedView_HandlePanZoom(&tv,window.Strokes,(Vec2){ GetMouse().x,GetMouse().y });

	if(Stroke(ALX_MOUSE_L).DOWN){
		Vec2 pW = TransformedView_ScreenWorldPos(&tv,GetMouse());
		if(pW.x>=0.0f && pW.x<SIZEX && pW.y>=0.0f && pW.y<SIZEY){
			world[(int)pW.y * SIZEX + (int)pW.x] = 1;
		}
	}
	if(Stroke(ALX_MOUSE_R).DOWN){
		Vec2 pW = TransformedView_ScreenWorldPos(&tv,GetMouse());
		if(pW.x>=0.0f && pW.x<SIZEX && pW.y>=0.0f && pW.y<SIZEY){
			world[(int)pW.y * SIZEX + (int)pW.x] = 0;
		}
	}

	if(Stroke(ALX_KEY_UP).DOWN) 		player.v.y = -SPEED;
	else if(Stroke(ALX_KEY_DOWN).DOWN) 	player.v.y = SPEED;
	//else 								player.v.y = 0.0f;

	if(Stroke(ALX_KEY_LEFT).DOWN) 		player.v.x = -SPEED;
	else if(Stroke(ALX_KEY_RIGHT).DOWN) player.v.x = SPEED;
	else 								player.v.x = 0.0f;


	player.v = Vec2_Add(player.v,Vec2_Mulf(player.a,w->ElapsedTime));
	Vec2 target = Vec2_Add(player.p,Vec2_Mulf(player.v,w->ElapsedTime));

	Clear(BLACK);

	Vector rects = Vector_New(sizeof(Rect));

	Vec2 dt = { 2.0f,2.0f };
	Vec2 tl = { F32_Min(player.p.x,target.x),F32_Min(player.p.y,target.y) };
	Vec2 br = { F32_Max(player.p.x,target.x),F32_Max(player.p.y,target.y) };
	tl = Vec2_Clamp(Vec2_Sub(tl,dt),(Vec2){0.0f,0.0f},(Vec2){SIZEX,SIZEY});
	br = Vec2_Clamp(Vec2_Add(br,dt),(Vec2){0.0f,0.0f},(Vec2){SIZEX,SIZEY});

	for(int i = 0;i<SIZEX * SIZEY;i++){
		char b = world[i];
		if(b!=0){
			const Vec2 p = { i % SIZEX,i / SIZEX };
			const Vec2 l = { 1.0f,1.0f };
			const Vec2 pS = TransformedView_WorldScreenPos(&tv,p);
			const Vec2 lS = TransformedView_WorldScreenLength(&tv,l);
			RenderRectWire(pS.x,pS.y,lS.x,lS.y,BLUE,1.0f);
		}
	}

	for(int i = tl.y;i<br.y+1;i++){
		for(int j = tl.x;j<br.x+1;j++){
			char b = world[i * SIZEX + j];
			if(b!=0){
				const Vec2 p = { j,i };
				const Vec2 l = { 1.0f,1.0f };
				Vector_Push(&rects,(Rect[]){ Rect_New(p,l) });
			}
		}
	}

	qsort(rects.Memory,rects.size,rects.ELEMENT_SIZE,(void*)Rect_Compare);

	for(int i = 0;i<rects.size;i++){
		Rect* r = (Rect*)Vector_Get(&rects,i);
		char type = Rect_Rect_RI_Solver(player.p,&target,player.l,r->p,r->d);
		if(type != INTERSECTION_NONE){
			if(type==INTERSECTION_PX) player.v.x = 0.0f;
			if(type==INTERSECTION_NX) player.v.x = 0.0f;
			if(type==INTERSECTION_PY) player.v.y = 0.0f;
			if(type==INTERSECTION_NY) player.v.y = 0.0f;

			const Vec2 pS = TransformedView_WorldScreenPos(&tv,r->p);
			const Vec2 lS = TransformedView_WorldScreenLength(&tv,r->d);
			RenderRect(pS.x,pS.y,lS.x,lS.y,MAGENTA);
		}else{
			const Vec2 pS = TransformedView_WorldScreenPos(&tv,r->p);
			const Vec2 lS = TransformedView_WorldScreenLength(&tv,r->d);
			RenderRectWire(pS.x,pS.y,lS.x,lS.y,YELLOW,1.0f);
		}
	}

	player.p = target;

	for(int i = 0;i<SIZEX * SIZEY;i++){
		char b = world[i];
		if(b!=0){
			// const Vec2 p = { i % SIZEX,i / SIZEX };
			// const Vec2 l = { 1.0f,1.0f };

			// const Vec2 pS = TransformedView_WorldScreenPos(&tv,p);
			// const Vec2 lS = TransformedView_WorldScreenLength(&tv,l);
			// RenderRectWire(pS.x,pS.y,lS.x,lS.y,BLUE,1.0f);

			// Vec2 pMouse = TransformedView_ScreenWorldPos(&tv,GetMouse());
			// Vec2 origin = player.p;
			// Vec2 dir = Vec2_Sub(pMouse,origin);
			
			// float t = 0.0f;
			// Vec2 cp = { 0.0f,0.0f };
			// char type = Rect_Ray_NearIntersection(origin,dir,p,l,&cp,&t);
			// if(type != INTERSECTION_NONE && t<=1.0f){
			// 	Vec2 pS = TransformedView_WorldScreenPos(&tv,origin);
			// 	Vec2 pT = TransformedView_WorldScreenPos(&tv,cp);
			// 	RenderLine(pS,pT,GREEN,1.0f);

			// 	const Vec2 n = Direction_Get(type);
			// 	Vec2 npS = TransformedView_WorldScreenPos(&tv,cp);
			// 	Vec2 npT = TransformedView_WorldScreenPos(&tv,Vec2_Add(cp,Vec2_Mulf(n,0.5f)));
			// 	RenderLine(npS,npT,YELLOW,1.0f);
			// 	break;
			// }else{
			// 	Vec2 pS = TransformedView_WorldScreenPos(&tv,origin);
			// 	Vec2 pT = TransformedView_WorldScreenPos(&tv,pMouse);
			// 	RenderLine(pS,pT,RED,1.0f);
			// }
		}
	}
	
	SRect_Render(&player);



	// String str = String_Format("P: X: %f, Y: %f",p.x,p.y);
	// RenderCStrSize(str.Memory,str.size,0.0f,0.0f,WHITE);
	// String_Free(&str);
}

void Delete(AlxWindow* w){
	
}

int main(){
    if(Create("Rect Ray Test",2500,1200,1,1,Setup,Update,Delete))
        Start();
    return 0;
}