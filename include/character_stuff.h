#pragma once
#include "string"
#include "helpers.h"
#include <vector>

struct CharacterSkills{
    int martial; //A measure of how good a person is in terms of warfare related stuff
    int intrigue; //A measure of how good a person can do back hand deals etc.
    int stewardship; //A measure of how good a person can manage finances etc.
    int diplomacy; // A measure of how good a person can do leadership tasks
    int knoledge; // A measre of how good a person can do research stuff etc.
};
struct Character{
    //should have basic stuff
    std::string name;
    int age;
    int gender;
    CharacterSkills skills;
    Character(std::string name,int age,int gender):name(name),age(age),gender(gender){}
};
inline std::vector<Character> CreateCharacters(
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
        int name_index = GetRandomIntValue(0, num_names - 1);
        int age = GetRandomIntValue(min_age, max_age);

        std::string name = names_list[name_index].get<std::string>();

        char_list.emplace_back(name, age, gender);
    }

    return char_list;
}