#include "../src/web_session.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace std::chrono;using websession::Tabs;auto t=Tabs::Time{};std::string a(32,'a'),b(32,'b');Tabs s;
 assert(!s.expired(t+hours(2)));s.touch(a,false,t);s.leave(a,t+seconds(1));assert(!s.expired(t+seconds(8)));assert(s.expired(t+seconds(9)));
 s.touch(b,false,t+seconds(5));assert(!s.expired(t+seconds(10)));s.touch(a,false,t+seconds(10));s.leave(b,t+seconds(11));assert(!s.expired(t+seconds(20)));s.leave(a,t+seconds(21));s.leave(a,t+seconds(25));assert(s.expired(t+seconds(29)));
 Tabs crashed;crashed.touch(a,false,t);assert(!crashed.expired(t+seconds(121)));assert(crashed.expired(t+seconds(129)));
 Tabs background;background.touch(a,true,t);assert(!background.expired(t+minutes(50)));background.leave(a,t+minutes(51));assert(background.expired(t+minutes(51)+seconds(8)));
 Tabs early;early.leave(a,t);assert(early.expired(t+seconds(8)));
 bool rejected=false;try{s.touch("bad",false,t);}catch(...){rejected=true;}assert(rejected);
 std::cout<<"PASS reload, multiple tabs, duplicate departure, background, crash expiry, early close and invalid id\n";
}
