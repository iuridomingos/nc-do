#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <ncurses.h>
#include <vector>
#include <string>
#include <algorithm>

using json = nlohmann::json;

std::string getDataPath()
{
    const char *home = std::getenv("HOME");
    if (!home)
        return "tasks.json"; // fallback if home isn't set

    std::filesystem::path p = home;
    p /= ".local/share/ncurses_todo_app";

    std::filesystem::create_directories(p);

    p /= "tasks.json";
    return p.string();
}

class TodoApp
{
    struct Task
    {
        std::string text;
        bool completed = false;
    };
    std::vector<Task> tasks;

public:
    void addTask(const std::string &desc) { tasks.push_back({desc, false}); }

    void toggleTask(size_t idx)
    {
        // tasks[idx].completed = !tasks[idx].completed is for making it opposite of what it is
        if (idx < tasks.size())
            tasks[idx].completed = !tasks[idx].completed;
    }

    void removeTask(size_t idx)
    {
        if (idx < tasks.size())
        {
            // telling the vector move forward as much as idx and erase that
            tasks.erase(tasks.begin() + idx);
        }
    }

    void removeAll()
    {
        tasks.clear();
    }

    void drawBox(WINDOW *win, const std::string &title)
    {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 0, 2, " %s ", title.c_str());
    }

    void renderList(WINDOW *win)
    {
        drawBox(win, "TASK LIST");
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            const char *status = tasks[i].completed ? "[X]" : "[ ]";
            mvwprintw(win, i + 1, 2, "%lu. %s %s", i, status, tasks[i].text.c_str());
            // c_str converts to format ncurses can handle, it's important
        }
        wrefresh(win);
    }

    void saveToFile()
    {
        std::string path = getDataPath();
        json j_list = json::array();

        for (const auto &t : tasks)
        {
            j_list.push_back({{"text", t.text},
                              {"completed", t.completed}});
        }

        std::ofstream file(path);
        file << j_list.dump(4);
    }

    void loadFromFile()
    {
        std::string path = getDataPath();
        std::ifstream file(path);

        if (!file.is_open())
            return;

        try
        {
            json j_list;
            file >> j_list;

            tasks.clear();

            for (const auto &item : j_list)
            {
                tasks.push_back({item.at("text").get<std::string>(),
                                 item.at("completed").get<bool>()});
            }
        }
        catch (const json::exception &e)
        {
            // if file corrupted
            tasks.clear();
        }
    }
};

int main()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    TodoApp app;
    app.loadFromFile();
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // grid: 1/3 for menu, 2/3 for list
    WINDOW *menuWin = newwin(maxY - 2, maxX / 3, 1, 1);
    WINDOW *listWin = newwin(maxY - 2, (2 * maxX / 3) - 2, 1, (maxX / 3) + 1);

    app.renderList(listWin);
    app.drawBox(menuWin, "MENU (q to quit)");
    mvwprintw(menuWin, 1, 2, "a: Add Task");
    mvwprintw(menuWin, 2, 2, "t: Complete");
    mvwprintw(menuWin, 3, 2, "r: Remove");
    mvwprintw(menuWin, 4, 2, "c: Remove All");
    wrefresh(menuWin);

    int ch;
    while ((ch = wgetch(menuWin)) != 'q')
    {
        if (ch == 'c')
            app.removeAll();

        if (ch == 'a' || ch == 't' || ch == 'r')
        {
            echo();
            curs_set(1); // show cursor only when typing
            char buf[32];
            mvwgetnstr(menuWin, 5, 2, buf, 31);
            // printing spaces over line 5 to erase the input
            mvwprintw(menuWin, 5, 2, "                                 ");

            std::string input(buf);

            bool isNumeric = !input.empty() && std::all_of(input.begin(), input.end(), ::isdigit);

            if (ch == 'a')
                app.addTask(input);
            else if (isNumeric && ch == 't')
                app.toggleTask(std::stoi(input));
            else if (isNumeric && ch == 'r')
                app.removeTask(std::stoi(input));

            noecho();
            curs_set(0);
        }
        app.renderList(listWin);
        wrefresh(menuWin);
    }
    app.saveToFile();
    endwin();
    return 0;
}