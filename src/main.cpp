#include <iostream>
#include "raylib.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <vector>
using json = nlohmann::json;
json LoadJson(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Failed to open JSON file");

    json data;
    file >> data;

    return data;
}
//First I'll start defining characters
struct Character{
    //should have basic stuff
    std::string name;
    int age;
    int gender;
    Character(std::string name,int age,int gender):name(name),age(age),gender(gender){}
};
std::vector<Character> CreateCharacters(
    const json& names,
    const std::string& origin,
    int gender,
    int count,
    int min_age = 10,
    int max_age = 100
)
{
    if (gender != 0 && gender != 1)
        throw std::invalid_argument("Invalid gender");

    const std::string gender_key = gender == 0 ? "masculine" : "feminine";

    const auto& names_list = names.at(origin).at(gender_key).at("names");

    if (!names_list.is_array() || names_list.empty())
        throw std::runtime_error("No names available");

    int num_names = static_cast<int>(names_list.size());

    std::vector<Character> char_list;

    for (int i = 0; i < count; i++)
    {
        int name_index = GetRandomValue(0, num_names - 1);
        int age = GetRandomValue(min_age, max_age);

        std::string name = names_list[name_index].get<std::string>();

        char_list.emplace_back(name, age, gender);
    }

    return char_list;
}
//With this then I can first make a Character generator
int main(){
    InitWindow(1200,800,"NPC system");
    json names=LoadJson("data/names.json");
    std::vector<Character>characters;
    characters=CreateCharacters(names,"welsh",0,10);
    std::cout<<"Name: "<<characters[0].name<<" ";
    std::cout<<"Age: "<<characters[0].age<<" ";
    if(characters[0].gender==0){
        std::cout<<"Gender: Male"<<std::endl;
    }
    
    return 0;
}