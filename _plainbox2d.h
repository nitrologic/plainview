/*

plainbox2d A box2d simulation engine
Copyright © 2023 Simon Armstrong
All Rights Reserved

// socket code courtesy github.com/nitrologic/nitro/nitro/box2dsim.h

*/

#include <box2d/box2d.h>
#include <vector>

#include "platform.h"

#define WorldScale 10.0f
#define ScreenScale 16

typedef std::vector<float> Points;

class Box2DBody{

public:
	b2Body *b2dbody;
	int id;
	float x;
	float y;
	float angle;
	float speed;
	Points shape;

	Box2DBody(b2Body *b2dbody,Points shape0,int id0):b2dbody(b2dbody){
		id=id0;
//		b2dbody->userData = b2BodyUserData();
//		b2dbody->SetUserData((void*)id);
		shape=shape0;
		updateBody();
	}

	void applyDrag(float drag){
		const b2Vec2& velocity=b2dbody->GetLinearVelocity();
		float fx=velocity.x*drag;
		float fy=velocity.y*drag;
		b2Vec2 force(fx,fy);
		b2Vec2 center = b2dbody->GetWorldCenter();
		b2dbody->ApplyForce(force,center,true);
	}

	float applyGravity(Box2DBody *star,float mass){
		float dx=star->x-x;
		float dy=star->y-y;
		float d=sqrt(dx*dx+dy*dy);

		float d3=mass/d*d*d;
		b2Vec2 force(d3*dx,d3*dy);
		b2Vec2 center = b2dbody->GetWorldCenter();
		b2dbody->ApplyForce(force,center,true);

		if (d<12){
			applyDrag(-5.0);
		}

		const b2Vec2& velocity=b2dbody->GetLinearVelocity();
		float speed=sqrt(velocity.x*velocity.x+velocity.y*velocity.y);
//		std::cout << "contact: distance:" << d << "speed:" << speed << std::endl;
		return d;
	}

	void applyForce(float angle,float force,int dist){
			
		float ax=-sin(angle);
		float ay=cos(angle);

		float fx=force*ax;
		float fy=force*ay;
		
		b2Vec2 center = b2dbody->GetWorldCenter();
		float px=center.x-ax*dist;
		float py=center.y-ay*dist;

		b2Vec2 forcevector(-fx,-fy);
		b2Vec2 point(px,py);

		b2dbody->ApplyForce(forcevector,point,true);
//		body->ApplyLinearImpulse(forcevector,point,true);
	}

	bool updateBody(){
		if(b2dbody){
			b2Vec2 position = b2dbody->GetPosition();
			x=position.x*WorldScale;
			y=position.y*WorldScale;
			angle=b2dbody->GetAngle();
			const b2Vec2 &v=b2dbody->GetLinearVelocity();
			speed=sqrt(v.x*v.x+v.y*v.y);
			return true;
		}
		return false;
	}

};

class Contact{
public:
	bool inside;
	Box2DBody *a;
	Box2DBody *b;
	double time;
	double tangentSpeed;
	Contact(bool c,Box2DBody *b0,Box2DBody *b1,double t,double ts):
		inside(c),a(b0),b(b1),time(t),tangentSpeed(ts){
	}
};

typedef std::vector<Contact> ContactList;

class Box2DSim:public b2ContactListener{
	b2World world;
	float timeStep;
	int32 velocityIterations;
	int32 positionIterations;
	float scale;

	std::map<b2Body *,Box2DBody*> bodyMap;

public:

	ContactList contacts;

	void flushContacts(){
		ContactList::iterator it=contacts.begin();
		while(it!=contacts.end()){
			if(!it->inside){
				it=contacts.erase(it);
			}else{
				it++;
			}
		}
	}

	void removeBody(Box2DBody *body){
		b2Body *b=body->b2dbody;
		if(b){
			world.DestroyBody(b);
			body->b2dbody=nullptr;
		}
	}


	Box2DSim():world(b2Vec2(0.0f,0.0f)){
		scale = 1.0f / WorldScale;
		timeStep = 1.0f / 60.0f;
		velocityIterations = 30;	//6
		positionIterations = 15;    //2
		world.SetContactListener(this);
	}

	virtual void BeginContact(b2Contact* contact){
		b2Fixture *f0=contact->GetFixtureA();
		b2Fixture *f1=contact->GetFixtureB();

		b2Body*b0=f0->GetBody();
		b2Body*b1=f1->GetBody();

		Box2DBody *a=bodyMap[b0];
		Box2DBody *b=bodyMap[b1];

		double tspeed=contact->GetTangentSpeed();
		double t= cpuTime();

		contacts.push_back(Contact(true,a,b,t,tspeed));
	}

	virtual void EndContact(b2Contact* contact){

		b2Fixture *f0=contact->GetFixtureA();
		b2Fixture *f1=contact->GetFixtureB();

		b2Body*b0=f0->GetBody();
		b2Body*b1=f1->GetBody();

		Box2DBody *a=bodyMap[b0];
		Box2DBody *b=bodyMap[b1];

		for(ContactList::iterator it=contacts.begin();it!=contacts.end();it++){
			Contact &c=*it;
			if(c.a==a && c.b==b){
				c.inside=false;
			}
		}
	}


	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
	}

	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){
	}

	void step(){
		world.Step(timeStep, velocityIterations, positionIterations);
	}

	Box2DBody *addBody(b2Body *body,Points points,int id){
		Box2DBody *result=new Box2DBody(body,points,id);
		bodyMap[body]=result;
		return result;
	}

	Box2DBody *addBox(float x,float y,float w,float h,int id){
		b2BodyDef bodyDef;
		bodyDef.position.Set(x*scale,y*scale);
		b2Body *body = world.CreateBody(&bodyDef);
		b2PolygonShape box;
		box.SetAsBox(w*scale*0.5,h*scale*0.5);
//		body->CreateFixture(&box, 0.0f);
		b2FixtureDef fixtureDef;
		fixtureDef.restitution = 0.5f;
		fixtureDef.shape = &box;
		fixtureDef.filter.categoryBits=WallBit;
		body->CreateFixture(&fixtureDef);
		return addBody(body,Points({w,h}),id);
	}

	enum {
		HoleBit=1,
		BallBit=2,
		WallBit=4,
		RailBit=8
	};

	Box2DBody *addHole(float x,float y,float rim,float hole,int id){
		b2BodyDef bodyDef;
		bodyDef.position.Set(x*scale,y*scale);
		b2Body *body = world.CreateBody(&bodyDef);
		b2CircleShape circle;
		circle.m_radius = rim*scale;
		b2FixtureDef fixtureDef;
		fixtureDef.isSensor=true;
		fixtureDef.shape = &circle;
		fixtureDef.filter.categoryBits=HoleBit;
		body->CreateFixture(&fixtureDef);
		return addBody(body,Points({rim,hole}),id);
	}

	Box2DBody *addRail(float x,float y,float r,int a,int b,int id){
		b2BodyDef bodyDef;
		bodyDef.position.Set(x*scale,y*scale);
		b2Body *body = world.CreateBody(&bodyDef);

		int32 count=b-a;
		std::vector<float> shape;
		for(int i=0;i<count;i++){
			float px=r*sin((a+i)*M_PI/16);
			float py=r*cos((a+i)*M_PI/16);
			shape.push_back(px);
			shape.push_back(py);
		}

		b2ChainShape chain;
		b2Vec2 points[1024];
		for(int i=0;i<count;i++){
			float sx=shape[i*2+0];
			float sy=shape[i*2+1];
			float px=scale*sx;
			float py=scale*sy;
			points[i].Set(px,py);
		}
//		chain.CreateChain(points,count);
		chain.CreateLoop(points, count);

		b2FixtureDef fixtureDef;
		fixtureDef.density = 0.1f;
		fixtureDef.filter.categoryBits=RailBit;
		fixtureDef.shape = &chain;
		body->CreateFixture(&fixtureDef);

		return addBody(body,shape,id);
	}

	Box2DBody *addBall(float x,float y,float r,int id){
		b2BodyDef bodyDef;

		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(x*scale,y*scale);
		bodyDef.linearDamping=1.0f;
		bodyDef.angularDamping=1.0f;
		bodyDef.bullet=true;

		b2Body *body = world.CreateBody(&bodyDef);

		b2CircleShape circle;
//		circle.m_p.Set(2.0f, 3.0f);
		circle.m_radius = r*scale;

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &circle;

		fixtureDef.density = 0.2f;
		fixtureDef.restitution = 0.82f;

		fixtureDef.friction = 0.92f;
		fixtureDef.filter.categoryBits=BallBit;
		fixtureDef.filter.maskBits=BallBit|WallBit|HoleBit|RailBit;

		body->CreateFixture(&fixtureDef);

		return addBody(body,Points({r}),id);
	}

};

typedef std::vector<Box2DBody*> BodyList;
