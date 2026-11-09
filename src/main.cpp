#include <iostream>
#include "raylib.h"
#include <vector>
#include "character_stuff.h"
#include "helpers.h"
//First I'll start defining characters
//With this then I can first make a Character generator
int main(){
    json names=LoadJson("data/names.json");
    std::vector<Character>characters;
    characters=CreateCharacters(names,"welsh", 1,10);
    for(int i=0;i<characters.size();i++){
        std::cout<<TextFormat("Name:%s Age: %d  Gender: %s",characters[i].name.c_str(),
        characters[i].age,
        characters[i].gender==0?"Male":"Female")<<std::endl;   
    }
    return 0;
}