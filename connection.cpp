#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using namespace std;

using json = nlohmann::json;

// Order Info to get OrderId
struct Order
{
    string orderId;
    string type;
    double amount;
    string symbol;
    string timestamp;
};

string API_URL;
string CLIENT_ID;
string CLIENT_SECRET;

// Storing the orders in a vector
vector<Order> orders;

void loadConfig(const string &filename)
{
    ifstream file(filename);
    nlohmann::json config_json;
    file >> config_json;

    API_URL = config_json["API"]["API_URL"];
    CLIENT_ID = config_json["API"]["CLIENT_ID"];
    CLIENT_SECRET = config_json["API"]["CLIENT_SECRET"];

    // cout << API_URL << "" << CLIENT_ID << " " << CLIENT_SECRET << endl;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

string placeOrder(const string &accessToken, const string &symbol, double amount, const string &type, const string &orderType)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
        return "cURL initialization failed";
    }

    string url;

    if (orderType == "buy")
    {
        url = API_URL + "private/buy?amount=" + to_string(amount) +
              "&instrument_name=" + symbol +
              "&type=" + type;
    }
    else if (orderType == "sell")
    {
        url = API_URL + "private/sell?amount=" + to_string(amount) +
              "&instrument_name=" + symbol +
              "&type=" + type;
    }
    else
    {
        cerr << "Invalid order type!" << endl;
        return "Invalid order type";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    // cout << "Order Response: " << readBuffer << endl;

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("result") && jsonResponse["result"].contains("order"))
        {
            string orderId = jsonResponse["result"]["order"]["order_id"];
            // cout << "Order ID: " << orderId << endl;

            orders.push_back({orderId, orderType, amount, symbol, "Timestamp here"});
            return orderId;
        }
        else
        {
            cerr << "Invalid response: " << jsonResponse.dump() << endl;
            return "No order ID received";
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
        return "JSON parse error";
    }
}

string cancelOrder(const string &accessToken, const string &orderId)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
        return "cURL initialization failed";
    }

    string url = API_URL + "private/cancel?order_id=" + orderId;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("error") && jsonResponse["error"]["message"] == "not_open_order")
        {
            cout << "Order cannot be canceled because it is not an open order." << endl;
        }
        else
        {
            cout << "Order " << jsonResponse["result"]["order_state"] << endl;
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }

    return readBuffer;
}

string modifyOrder(const string &accessToken, const string &orderId, double newAmount, double newPrice)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
        return "cURL initialization failed";
    }

    string url = API_URL + "private/edit?order_id=" + orderId +
                 "&amount=" + to_string(newAmount) +
                 "&price=" + to_string(newPrice);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("error") && jsonResponse["error"]["message"] == "not_open_order")
        {
            cout << "Order cannot be modified because it is not an open order." << endl;
        }
        else
        {
            if (jsonResponse.contains("result") && jsonResponse["result"].contains("order"))
            {
                string orderId = jsonResponse["result"]["order"]["order_id"];
                // cout << "Order ID: " << orderId << endl; // Print order ID
                string orderType = jsonResponse["result"]["order"]["order_type"];

                double amount = jsonResponse["result"]["order"]["amount"];

                string symbol = jsonResponse["result"]["order"]["instrument_name"];

                orders.push_back({orderId, orderType, amount, symbol, "time rn"});
                return orderId;
            }
            else
            {
                cerr << "Invalid response: " << jsonResponse.dump() << endl;
                return "No order ID received";
            }
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }

    return readBuffer;
}

void getOrderBook(const string &symbol)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
    }

    string url = API_URL + "public/get_order_book?instrument_name=" + symbol;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("result"))
        {
            const auto &orderBook = jsonResponse["result"];
            cout << "Order Book for " << orderBook["instrument_name"] << ":\n"
                 << "Best Bid: " << orderBook["best_bid_price"] << " (Amount: " << orderBook["best_bid_amount"] << ")\n"
                 << "Best Ask: " << orderBook["best_ask_price"] << " (Amount: " << orderBook["best_ask_amount"] << ")\n"
                 << "Last Price: " << orderBook["last_price"] << "\n"
                 << "Mark Price: " << orderBook["mark_price"] << "\n"
                 << "Volume: " << orderBook["stats"]["volume"] << "\n"
                 << "Low: " << orderBook["stats"]["low"] << "\n"
                 << "High: " << orderBook["stats"]["high"] << "\n\n";
        }
        else
        {
            cerr << "No order book data available\n";
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
}

void getCurrentPositions(const string &accessToken)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
    }

    string url = API_URL + "private/get_positions?";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("result"))
        {
            cout << "Current Positions:\n";
            for (const auto &position : jsonResponse["result"])
            {
                cout << "Instrument: " << position["instrument_name"] << "\n"
                     << "Direction: " << position["direction"] << "\n"
                     << "Size: " << position["size"] << "\n"
                     << "Average Price: " << position["average_price"] << "\n"
                     << "Mark Price: " << position["mark_price"] << "\n"
                     << "Floating P/L: " << position["floating_profit_loss"] << "\n"
                     << "Leverage: " << position["leverage"] << "\n\n";
            }
        }
        else
        {
            cerr << "No position data available\n";
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
}

string getAccessToken()
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize cURL!" << endl;
        return "";
    }

    string url = API_URL + "public/auth?client_id=" + CLIENT_ID +
                 "&client_secret=" + CLIENT_SECRET +
                 "&grant_type=client_credentials";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "cURL Error: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);

    try
    {
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("result") && jsonResponse["result"].contains("access_token"))
        {
            return jsonResponse["result"]["access_token"];
        }
        else
        {
            cerr << "Invalid response: " << jsonResponse.dump() << endl;
            return "";
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "JSON Parse Error: " << e.what() << endl;
        return "";
    }
}

void viewOrders()
{
    if (orders.empty())
    {
        cout << "No orders found." << endl;
        return;
    }

    cout << "Current Orders:" << endl;
    for (const auto &order : orders)
    {
        cout << "Order ID: " << order.orderId
             << ", Type: " << order.type
             << ", Amount: " << order.amount
             << ", Symbol: " << order.symbol
             << endl;
    }
}

int main()
{
    loadConfig("config.json");
    string accessToken = getAccessToken();

    if (accessToken.empty())
    {
        cout << "Failed to retrieve access token." << endl;
        return 1;
    }

    while (true)
    {
        cout << "\n--- Deribit Trading System ---" << endl;
        cout << "1. Place Order" << endl;
        cout << "2. Cancel Order" << endl;
        cout << "3. Modify Order" << endl;
        cout << "4. View Order Book" << endl;
        cout << "5. View Current Positions" << endl;
        cout << "6. View Orders" << endl;
        cout << "7. Exit" << endl;
        cout << "Choose an option (1-7): ";
        int choice;
        cin >> choice;

        if (choice == 7)
        {
            cout << "Exiting..." << endl;
            break;
        }

        switch (choice)
        {
        case 1:
        {
            // Place an order
            string symbol, type, orderType;
            double amount;
            cout << "Enter symbol (e.g., BTC-PERPETUAL): ";
            cin >> symbol;
            cout << "Enter amount: ";
            cin >> amount;
            cout << "Enter order type (e.g., market, limit): ";
            cin >> type;
            cout << "Buy or sell: ";
            cin >> orderType;
            string orderId = placeOrder(accessToken, symbol, amount, type, orderType);
            cout << "Order placed with ID: " << orderId << endl;
            break;
        }
        case 2:
        {
            // Cancel an order
            string orderId;
            cout << "Enter order ID to cancel: ";
            cin >> orderId;
            string response = cancelOrder(accessToken, orderId);
            // cout << "Cancel order response: " << response << endl;
            break;
        }
        case 3:
        {
            // Modify an order
            string orderId;
            double newAmount, newPrice;
            cout << "Enter order ID to modify: ";
            cin >> orderId;
            cout << "Enter new amount: ";
            cin >> newAmount;
            cout << "Enter new price: ";
            cin >> newPrice;
            string response = modifyOrder(accessToken, orderId, newAmount, newPrice);
            // cout << "Modify order response: " << response << endl;
            break;
        }
        case 4:
        {
            // View order book
            string symbol;
            cout << "Enter symbol (e.g., BTC-PERPETUAL): ";
            cin >> symbol;
            getOrderBook(symbol);

            break;
        }
        case 5:
        {
            // View current positions
            getCurrentPositions(accessToken);

            break;
        }
        case 6:
        {
            // View orders
            viewOrders();
            break;
        }
        default:
        {
            cout << "Invalid choice, please try again." << endl;
            break;
        }
        }
    }

    return 0;
}
