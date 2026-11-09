#include "world.h"
int main(){
    json names=LoadJson("data/names.json");
    json settings=LoadJson("data/world.json");
    World world{names,settings};
    return 0;
}