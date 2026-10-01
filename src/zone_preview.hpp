#pragma once
void drawZonePreview(){
    if(!showZone||model.points.empty())return;
    Region r=region.enabled?region:comparison?comparison->region:bounds(model);
    if(!r.enabled)r=bounds(model);
    // Unbounded Z limits of an XY selection use the measured vertical extent.
    double z0=std::max(r.z0,double(model.lo.z)),z1=std::min(r.z1,double(model.hi.z));
    if(z1<=z0){z0=model.lo.z;z1=model.hi.z;}
    std::array<Point,8> p;for(int i=0;i<8;i++)p[i]={float(i&1?r.x1:r.x0),float(i&2?r.y1:r.y0),float(i&4?z1:z0)};
    glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glLineWidth(1);glEnable(GL_LINE_STIPPLE);glLineStipple(1,0x5555);glColor3f(.55f,.55f,.55f);glBegin(GL_LINES);
    for(int i=0;i<8;i++)for(int bit:{1,2,4})if(!(i&bit)){auto a=p[i],b=p[i|bit];glVertex3f(a.x,a.y,a.z);glVertex3f(b.x,b.y,b.z);}glEnd();glDisable(GL_LINE_STIPPLE);
    const std::array<std::array<float,3>,3> colors={std::array<float,3>{1,.22f,.18f},{.1f,1,.1f},{.25f,.35f,1}};
    double lo[]={r.x0,r.y0,z0},hi[]={r.x1,r.y1,z1};const wchar_t* axis[]={L"X",L"Y",L"Z"};
    for(int d=0;d<3;d++){auto a=p[0],b=p[1<<d];glColor3fv(colors[d].data());glBegin(GL_LINES);glVertex3f(a.x,a.y,a.z);glVertex3f(b.x,b.y,b.z);glEnd();
        if(textList)for(int tick=0;tick<=2;tick++){float t=tick*.5f;Point q{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};std::wostringstream s;s<<axis[d]<<L": "<<std::fixed<<std::setprecision(2)<<(lo[d]+(hi[d]-lo[d])*t);auto text=s.str();glRasterPos3f(q.x,q.y,q.z);glBitmap(0,0,0,0,8,float(-14-d*16),nullptr);glListBase(textList);glCallLists(GLsizei(text.size()),GL_UNSIGNED_SHORT,text.data());}
    }
    glEnable(GL_DEPTH_TEST);
}
