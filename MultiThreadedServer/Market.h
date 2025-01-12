#pragma once
#pragma warning(disable : 4996)
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <random>
#include <string>

//Hashing function to hash user passwords and compare with the passwords stored in the database
struct Stonks
{
    std::string Name;
    float price;

    Stonks()
    {

    }

    Stonks(std::string name, float Price)
    {
        Name = name;
        price = Price;
    }
};

struct Shares
{
    std::string Share_Name;
    int Share_Quantity;

    Shares()
    {

    }

    Shares(std::string Name, int Quantity)
    {
        Share_Name = Name;
        Share_Quantity = Quantity;
    }
};

struct Customer
{
    int id;
    std::string Name;
    std::string password;
    float Balance;
    bool logged_in = false;

    std::vector<Shares> Customershares;
};

std::vector<Stonks> Market;//Contains Stock names and prices

//overload for the standard hash function
namespace std
{
    template<>struct hash<Stonks>
    {
        size_t operator()(const Stonks& key)
        {
            return hash<std::string>()(key.Name);
        }
    };

    template<>struct hash<Customer>
    {
        size_t operator()(const Customer& key)
        {
            return hash<std::string>()(key.Name);
        }
    };
}

std::unordered_map<std::string, Stonks> Stock_Map; //Stock Map

std::unordered_map<std::string, Customer> Cus_Map; //Customer Map

//Function to authenticate the username and password received from clients
const Customer Authenticate(std::string Username, std::string Password) 
{
    const auto& account = Cus_Map;
    Customer Null;
    Null.id = -1;
    if (account.find(Username) != account.end())
    {
        const Customer& Acc = account.at(Username);
        if (Acc.password == Password)
        {
            return Acc;
        }
        return Null;
    }
    else
    {
        return Null;
    }
}

//Function which calculates and checks if the customer has enough balance to buy shares
bool BuyStock(int Qant, std::string StockName, Customer& Auth)
{
    if ((Stock_Map[StockName].price * Qant) <= Auth.Balance)
    {
        for (int i = 0; i < Market.size(); i++)
        {
            if (Market[i].Name == StockName)
            {
                for (int j = 0; j < Auth.Customershares.size(); j++)
                {
                    if (Auth.Customershares[j].Share_Name == Market[i].Name)
                    {
                        Auth.Customershares[j].Share_Quantity += Qant;
                        Auth.Balance -= (Market[i].price * Qant);
                        return true;
                    }
                }
                Auth.Customershares.push_back(Shares(StockName, Qant));
                Auth.Balance -= (Market[i].price * Qant);
                return true;
            }
        }
    }
    else
        return false;
}

//Function which calculates and checks if the customer has enough shares to sell
bool SellStock(int quant, std::string StockName, Customer& Auth)
{
    for (int i = 0; i < Market.size(); i++)
    {
        if (Market[i].Name == StockName)
        {
            for (int j = 0; j < Auth.Customershares.size(); j++)
            {
                if (Auth.Customershares[j].Share_Name == Market[i].Name)
                {
                    if (Auth.Customershares[j].Share_Quantity >= quant)
                    {
                        Auth.Customershares[j].Share_Quantity -= quant;
                        Auth.Balance += (quant * Market[i].price);
                        if (Auth.Customershares[j].Share_Quantity == 0)
                        {
                            Auth.Customershares.erase(Auth.Customershares.begin() + j);
                            return true;
                        }
                        return true;
                    }
                    return false;
                }
            }
        }
    }
}

