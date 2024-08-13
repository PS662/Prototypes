#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <thread>
#include <random>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

const int NUM_SEATS = 120;
const int SEATS_PER_ROW = 10;
const std::string TRIP_NAME = "QA101";
const int POOL_SIZE = 10;

class ConnectionPool
{
public:
    ConnectionPool(const std::string &conninfo, int pool_size) : conninfo(conninfo), pool_size(pool_size)
    {
        for (int i = 0; i < pool_size; ++i)
        {
            connections.push(std::make_unique<pqxx::connection>(conninfo));
        }
    }

    std::unique_ptr<pqxx::connection> get()
    {
        std::unique_lock<std::mutex> lock(mtx);
        while (connections.empty())
        {
            cv.wait(lock);
        }
        auto conn = std::move(connections.front());
        connections.pop();
        return conn;
    }

    void put(std::unique_ptr<pqxx::connection> conn)
    {
        std::unique_lock<std::mutex> lock(mtx);
        connections.push(std::move(conn));
        cv.notify_one();
    }

private:
    std::string conninfo;
    int pool_size;
    std::queue<std::unique_ptr<pqxx::connection>> connections;
    std::mutex mtx;
    std::condition_variable cv;
};

class UserInfo
{
public:
    int user_id;
    std::string name;

    UserInfo(int uid, std::string uname) : user_id(uid), name(uname) {}
};

class SeatInfo
{
public:
    int seat_id;
    std::string seat_name;

    SeatInfo() : seat_id(-1), seat_name("") {}
    SeatInfo(int sid, std::string sname) : seat_id(sid), seat_name(sname) {}
};

std::string generate_random_name(const std::vector<std::string> &first_names, const std::vector<std::string> &last_names)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> first_dist(0, first_names.size() - 1);
    std::uniform_int_distribution<> last_dist(0, last_names.size() - 1);

    return first_names[first_dist(gen)] + " " + last_names[last_dist(gen)];
}

void populate_db(ConnectionPool &pool)
{
    auto conn = pool.get();
    try
    {
        pqxx::work txn(*conn);

        std::vector<std::string> first_names = {"John", "Jane", "Alice", "Bob", "Charlie", "David", "Eva", "Frank", "Grace", "Hank", "Ivy", "Jack"};
        std::vector<std::string> last_names = {"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis", "Martinez", "Lopez"};

        for (int i = 0; i < NUM_SEATS; i++)
        {
            std::string random_name = generate_random_name(first_names, last_names);
            txn.exec0("INSERT INTO users (name) VALUES (" + txn.quote(random_name) + ")");
        }

        txn.exec0("INSERT INTO trips (name) VALUES (" + txn.quote(TRIP_NAME) + ")");

        char rows[] = {'A', 'B', 'C', 'D', 'E', 'F'};
        int trip_id = 1;
        for (int i = 1; i <= NUM_SEATS / 6; i++)
        {
            for (char row : rows)
            {
                std::string seat_name = std::to_string(i) + "-" + row;
                txn.exec0("INSERT INTO seats (name, trip_id) VALUES (" + txn.quote(seat_name) + ", " + std::to_string(trip_id) + ")");
            }
        }

        txn.commit();
        std::cout << "Database populated successfully."
                  << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
}

void deleteRows(ConnectionPool &pool)
{
    auto conn = pool.get();
    try
    {
        pqxx::work txn(*conn);
        txn.exec0("TRUNCATE TABLE seats RESTART IDENTITY CASCADE");
        txn.exec0("TRUNCATE TABLE users RESTART IDENTITY CASCADE");
        txn.exec0("TRUNCATE TABLE trips RESTART IDENTITY CASCADE");
        txn.commit();
        std::cout << "All rows deleted and sequences reset successfully."
                  << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
}

UserInfo getUserDetails(int user_id, ConnectionPool &pool)
{
    auto conn = pool.get();
    UserInfo user_info(user_id, "");
    try
    {
        pqxx::work txn(*conn);
        pqxx::result R = txn.exec("SELECT name FROM users WHERE user_id = " + txn.quote(user_id));

        if (!R.empty())
        {
            user_info.name = R[0]["name"].as<std::string>();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
    return user_info;
}

SeatInfo book_approach1(UserInfo user_info, ConnectionPool &pool)
{
    auto conn = pool.get();
    SeatInfo seat_info;
    try
    {
        pqxx::work txn(*conn);

        pqxx::result R = txn.exec("SELECT seat_id, name, trip_id, user_id FROM seats WHERE trip_id = 1 AND user_id IS NULL ORDER BY seat_id LIMIT 1");

        if (!R.empty())
        {
            seat_info.seat_id = R[0]["seat_id"].as<int>();
            seat_info.seat_name = R[0]["name"].as<std::string>();

            txn.exec0("UPDATE seats SET user_id = " + std::to_string(user_info.user_id) + " WHERE seat_id = " + std::to_string(seat_info.seat_id));
            txn.commit();
        }
        else
        {
            txn.abort();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
    return seat_info;
}

SeatInfo book_approach2(UserInfo user_info, ConnectionPool &pool)
{
    auto conn = pool.get();
    SeatInfo seat_info;
    try
    {
        pqxx::work txn(*conn);

        pqxx::result R = txn.exec("SELECT seat_id, name, trip_id, user_id FROM seats WHERE trip_id = 1 AND user_id IS NULL ORDER BY seat_id LIMIT 1 FOR UPDATE");

        if (!R.empty())
        {
            seat_info.seat_id = R[0]["seat_id"].as<int>();
            seat_info.seat_name = R[0]["name"].as<std::string>();

            txn.exec0("UPDATE seats SET user_id = " + std::to_string(user_info.user_id) + " WHERE seat_id = " + std::to_string(seat_info.seat_id));
            txn.commit();
        }
        else
        {
            txn.abort();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
    return seat_info;
}

SeatInfo book_approach3(UserInfo user_info, ConnectionPool &pool)
{
    auto conn = pool.get();
    SeatInfo seat_info;
    try
    {
        pqxx::work txn(*conn);

        pqxx::result R = txn.exec("SELECT seat_id, name, trip_id, user_id FROM seats WHERE trip_id = 1 AND user_id IS NULL ORDER BY seat_id LIMIT 1 FOR UPDATE SKIP LOCKED");

        if (!R.empty())
        {
            seat_info.seat_id = R[0]["seat_id"].as<int>();
            seat_info.seat_name = R[0]["name"].as<std::string>();

            txn.exec0("UPDATE seats SET user_id = " + std::to_string(user_info.user_id) + " WHERE seat_id = " + std::to_string(seat_info.seat_id));
            txn.commit();
        }
        else
        {
            txn.abort();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
    return seat_info;
}

void PrintSeats(ConnectionPool &pool, const std::vector<SeatInfo> &seats, const std::vector<UserInfo> &users)
{
    for (int i = 0; i < NUM_SEATS; ++i)
    {
        if (seats[i].seat_id != -1)
        {
            std::cout << users[i].name << " was assigned " << seats[i].seat_name << "\n";
        }
    }

    auto conn = pool.get();
    try
    {
        pqxx::work txn(*conn);
        pqxx::result R = txn.exec("SELECT user_id FROM seats ORDER BY seat_id");

        int count = 0;
        for (auto row : R)
        {
            if (row["user_id"].is_null())
            {
                std::cout << ".";
            }
            else
            {
                std::cout << "x";
            }
            if (++count % SEATS_PER_ROW == 0)
            {
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    pool.put(std::move(conn));
}

std::vector<UserInfo> prepare_db(ConnectionPool &pool)
{
    deleteRows(pool);
    populate_db(pool);

    std::vector<UserInfo> users;
    for (int i = 1; i <= NUM_SEATS; ++i)
    {
        users.push_back(getUserDetails(i, pool));
    }
    return users;
}

void run(ConnectionPool &pool, int approach)
{
    std::chrono::milliseconds duration1{0}, duration2{0}, duration3{0};

    if (approach == 1 || approach == 0)
    {
        std::vector<UserInfo> users = prepare_db(pool);
        std::vector<std::thread> threads;
        std::vector<SeatInfo> seats(NUM_SEATS);
        std::cout << "Running Approach 1...\n";

        auto start_time = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_SEATS; ++i)
        {
            threads.push_back(std::thread([&seats, i, &pool, &users]()
                                          { seats[i] = book_approach1(users[i], pool); }));
        }

        for (auto &th : threads)
        {
            th.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        PrintSeats(pool, seats, users);
    }

    if (approach == 2 || approach == 0)
    {
        std::vector<UserInfo> users = prepare_db(pool);
        std::vector<std::thread> threads;
        std::vector<SeatInfo> seats(NUM_SEATS);
        std::cout << "Running Approach 2...\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_SEATS; ++i)
        {
            threads.push_back(std::thread([&seats, i, &pool, &users]()
                                          { seats[i] = book_approach2(users[i], pool); }));
        }

        for (auto &th : threads)
        {
            th.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        PrintSeats(pool, seats, users);
    }

    if (approach == 3 || approach == 0)
    {
        std::vector<UserInfo> users = prepare_db(pool);
        std::vector<std::thread> threads;
        std::vector<SeatInfo> seats(NUM_SEATS);
        std::cout << "Running Approach 3...\n";

        auto start_time = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_SEATS; ++i)
        {
            threads.push_back(std::thread([&seats, i, &pool, &users]()
                                          { seats[i] = book_approach3(users[i], pool); }));
        }

        for (auto &th : threads)
        {
            th.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        PrintSeats(pool, seats, users);
    }

    if (approach == 0)
    {
        std::cout << "Approach 1 completed in " << duration1.count() << " ms.\n";
        std::cout << "Approach 2 completed in " << duration2.count() << " ms.\n";
        std::cout << "Approach 3 completed in " << duration3.count() << " ms.\n";
    }
    else if (approach == 1)
    {
        std::cout << "Approach 1 completed in " << duration1.count() << " ms.\n";
    }
    else if (approach == 2)
    {
        std::cout << "Approach 2 completed in " << duration2.count() << " ms.\n";
    }
    else if (approach == 3)
    {
        std::cout << "Approach 3 completed in " << duration3.count() << " ms.\n";
    }
}

int main(int argc, char *argv[])
{
    try
    {
        std::string conninfo = "dbname=airline_checkin_testdb user=testuser password=Password123! host=localhost";
        ConnectionPool pool(conninfo, POOL_SIZE);

        int approach = 0; // Default is 0, which means run all approaches

        if (argc > 1)
        {
            std::string arg = argv[1];
            if (arg == "--approach1")
                approach = 1;
            else if (arg == "--approach2")
                approach = 2;
            else if (arg == "--approach3")
                approach = 3;
            else
            {
                std::cerr << "Invalid argument. Use --approach1, --approach2, --approach3, or none for all.\n";
                return 1;
            }
        }

        run(pool, approach);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
