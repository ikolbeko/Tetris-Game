#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
using namespace std;
using namespace sf;

int block[7][4][4] =      // Массив в котором хранятся виды блоков
{
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    
    1,0,0,0,
    1,1,0,0,
    0,1,0,0,
    0,0,0,0,
    
    0,1,0,0,
    1,1,0,0,
    1,0,0,0,
    0,0,0,0,
    
    1,1,0,0,
    1,1,0,0,
    0,0,0,0,
    0,0,0,0,
    
    1,0,0,0,
    1,1,0,0,
    1,0,0,0,
    0,0,0,0,
    
    1,0,0,0,
    1,0,0,0,
    1,1,0,0,
    0,0,0,0,
    
    0,1,0,0,
    0,1,0,0,
    1,1,0,0,
    0,0,0,0,
};
const Color color_map[] = {
    Color::Green, Color::Blue, Color::Red, Color::Yellow,        //  Цветовой массив
    Color::White, Color::Magenta, Color::Cyan
};
const int cell_size = 25;       // Размер одного кубика
const int w_cnt = 10;
const int h_cnt = 20;           // Размер игрового поля ( В кубиках)
int world[h_cnt][w_cnt] = { 0 };       //  Двумерный массив игрового пространства
float speed = 0.5;                 //  Начальная скорость игры ( изменяется по мере увеличения уровня)
int score = 0;           // Переменная хранящая набранные очки
int scoreCount = 0;
int level = 1;
bool GameOver = false;
string username;
struct userTable {       // Структура таблицы рекордов (используется для удобного считывания данных из файла с рекордами)
    string name;
    int scoreRecords;
};

int main(void)
{
    
    ContextSettings settings;
    settings.antialiasingLevel = 8;
    Clock clock;
    vector<userTable> base;
    
    // Музыка
    Music music;
    if (!music.openFromFile("src/korobushka.ogg")) {
        return EXIT_FAILURE;
    }
    
    music.openFromFile("src/korobushka.ogg");
    music.play();
    music.setLoop(true);
    music.setVolume(5);
    
    SoundBuffer rotateBuf;
    rotateBuf.loadFromFile("src/block-rotate.ogg");
    Sound rotateSound(rotateBuf);
    
    SoundBuffer cleanBuf;
    cleanBuf.loadFromFile("src/clear.wav");
    Sound cleanSound(cleanBuf);
    
    SoundBuffer downBuf;
    downBuf.loadFromFile("src/drop.wav");
    Sound downSound(downBuf);
    downSound.setVolume(30);
    
    sf::Font font;
    if (!font.loadFromFile("src/font.otf"))     // Проверка на отрытие шрифта
    {
        return EXIT_FAILURE;
    }
    
    font.loadFromFile("src/font.otf");              // Передаем нашему шрифту файл шрифта
    Text ScoreText("", font, 40);               // Создаем объект текст. закидываем в объект текст строку, шрифт, размер шрифта(в пикселях)
    ScoreText.setColor(Color::Red);         // Покрасили текст в красный
    ScoreText.setStyle(sf::Text::Bold);         // Жирный текст. по умолчанию он обычный
    
    Text LevelText("", font, 40);
    LevelText.setColor(Color::Red);
    LevelText.setStyle(sf::Text::Bold);
    
    ifstream read;
    read.open("src/Records.txt");         // чтение данных о рекордах из файла
    userTable temp;
    while (!read.eof()) {
        while (read >> temp.name >> temp.scoreRecords ) {
            base.push_back(temp);
        }
    }
    
    RenderWindow window(VideoMode(502, 505), "Tetris");     // Создание игрового окна
    RectangleShape cell(Vector2f(cell_size, cell_size));     // Задаем размер кубика
    
    int kind; // Хранит тип блока
    int cx; // Хранит текущую позицию по осям x y
    int cy;
    
    auto new_block = [&]()        // Функция генерации нового блока и задание начальных координат
    {
        kind = rand() % 7, cx = w_cnt / 2, cy = 0;
    };
    new_block();
    
    auto check_block = [&]()         // Функция проверки не задел ли блок стену или другие блоки
    {
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                if (block[kind][y][x] == 0) continue;
                if (x + cx < 0 || x + cx >= w_cnt || y + cy >= h_cnt) return false;     // Столкновение со стеной
                if (world[cy + y][cx + x]) return false;      // Столкновение с блоками
            }
        }
        return true;
    };
    
    auto LevelUp = [&]()     // Увеличение уровня
    {
        level++;
        speed = speed / 1.2;     // Ускорение игры
    };
    
    auto clear_lines = [&]()       // Очистка полной линии, неполная линия копируется и сохраняется, полная игрорируется
    {
        int to = h_cnt - 1;
        for (int from = h_cnt - 1; from >= 0; from--)  // Вертикальный перебор игрового поля
        {
            int cnt = 0;
            
            for (int x = 0; x < w_cnt; x++)  // Горизонтальный перебор
            {
                if (world[from][x])cnt++;
            }
            // Если линия не полная, копируем ее
            
            if(cnt == w_cnt) {         // Если нашли полную линию добавляем очки, и проигрываем музыку
                cleanSound.play();
                score++;
                scoreCount++;
                if (scoreCount == 10) {
                    LevelUp();
                    scoreCount = 0;
                }
            }
            
            if (cnt < w_cnt)
            {
                for (int x = 0; x < w_cnt; x++) world[to][x] = world[from][x];     // копирование
                to--;
            }
        }
    };
    
    auto clear = [&]()      //  Полная очистка от кубиков, используется при завершении игры
    {
        for (int from = h_cnt - 1; from >= 0; from--)  // Очистка поля от кубиков
        {
            for (int x = 0; x < w_cnt; x++)
            {
                world[from][x] = 0;
            }
        }
    };
    
    auto gameOver = [&]()        // Конец игры :)
    {
        for (int x = 0; x < w_cnt; x++)
        {
            if (world[0][x]) {               //  Если кубики добрались до верхней ячейки
                 
                userTable empty;      // Временная переменная
                empty.name = username;    // Передача данных в переменную
                empty.scoreRecords = score;
                base.push_back(empty);     // Добавление данных о набранных очках в базу
                
                sort(base.begin(), base.end(), [](const userTable& a, const userTable& b) -> bool {
                    return  a.scoreRecords > b.scoreRecords;
                });                                           //  Сортируем таблицу
                
                ofstream write;
                remove("src/Records.txt");
                write.open("src/Records.txt", ofstream::trunc);
                for (int i = 0; i < base.size(); i++)           //  Записываем данные в фаил
                {
                    write << base[i].name << " " << base[i].scoreRecords << "\n";
                }
                write.close();
                
                GameOver = true;         // Обнуляем все значения игры
                speed = 0.5;
                score = 0;
                scoreCount = 0;
                level = 1;
                clear();
            }
        }
    };
    
    auto go_down = [&]()      // Движение вниз
    {
        cy++;
        if (check_block() == false) // Если блок достиг дна
        {
            cy--;
            for (int y = 0; y < 4; y++)for (int x = 0; x < 4; x++)
            if (block[kind][y][x])
            {
                world[cy + y][cx + x] = kind + 1;
            }
            clear_lines();
            gameOver();
            //  Старт нового блока
            new_block();
            downSound.play();
            return false;
        }
        return true;
    };
    
    auto rotate = [&]()
    {
        int len = 0; // Проверка размера блока вращения
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                if (block[kind][y][x])
                {
                    len = max(max(x, y) + 1, len);
                }
            }
        }
        int d[4][4] = { 0 };
                            // Повернуть против часовой стрелки на 90 градусов
        for (int y = 0; y < len; y++)
        {
            for (int x = 0; x < len; x++)
            {
                if (block[kind][y][x]) d[len - 1 - x][y] = 1;
            }
        }
        
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                block[kind][y][x] = d[y][x];
            }
        }
        rotateSound.play();
        
    };
    
    auto draw_text = [&]()
    {
        std::ostringstream ScoreString;    // объявили переменную
        ScoreString << score;        //занесли в нее число очков, то есть формируем строку
        ScoreText.setString(" SCORE " + ScoreString.str());//задаем строку тексту и вызываем сформированную выше строку методом .str()
        ScoreText.setPosition(290, 350);//задаем позицию текста, отступая от центра камеры
        window.draw(ScoreText);//рисую этот текст
        
        std::ostringstream LevelString;
        LevelString << level;
        LevelText.setString(" Level " + LevelString.str());
        LevelText.setPosition(290, 210);
        window.draw(LevelText);
        
        RectangleShape H_line(Vector2f(256.f, 5.f));
        H_line.setFillColor(Color(15, 180, 140));
        H_line.move(0, 501);
        window.draw(H_line);
        
        RectangleShape W_line(Vector2f(501.f, 5.f));
        W_line.setFillColor(Color(15, 180, 140));
        W_line.rotate(90);
        W_line.move(256, 0);
        window.draw(W_line);
    };
    
    auto draw_world = [&]()     // Отрисовка мира
    {
        for (int y = 0; y < h_cnt; y++)for (int x = 0; x < w_cnt; x++)
        if (world[y][x])
        {
            cell.setFillColor(color_map[world[y][x] - 1]);
            cell.setOutlineThickness(1);
            cell.setOutlineColor(Color::Black);
            cell.setPosition(Vector2f(x*cell_size, y*cell_size));
            window.draw(cell);
        }
    };
    
    auto draw_block = [&]()
    {
        cell.setFillColor(color_map[kind]);
        for (int y = 0; y < 4; y++)for (int x = 0; x < 4; x++)
        if (block[kind][y][x])
        {
            cell.setPosition(Vector2f((cx + x)*cell_size, (cy + y)*cell_size));
            window.draw(cell);
        }
    };
    
    auto menu = [&]()
    {
        Text MenuText("TETRIS", font, 50);
        MenuText.setColor(Color::Red);
        MenuText.setStyle(sf::Text::Bold);
        MenuText.setPosition(160, 20);
        
        Text RecText("Records", font, 25);
        RecText.setColor(Color::Magenta);
        RecText.setStyle(sf::Text::Bold);
        RecText.setPosition(200, 90);
        
        Text KeyText("Press ENTER to start game", font, 20);
        KeyText.setColor(Color::Green);
        KeyText.setStyle(sf::Text::Bold);
        KeyText.setPosition(120, 450);
        int y = 130;
        
        if (base.size() != 0) {
            for (int d = 0; d < base.size(); d++) {
                ostringstream ost;
                ost << base[d].scoreRecords;
                
                Text recordsName(base[d].name, font, 25);
                recordsName.setColor(Color::Yellow);
                recordsName.setPosition(140, y);
                
                Text recordsScore(ost.str(), font, 25);
                recordsScore.setColor(Color::Red);
                recordsScore.setPosition(320, y);
                window.draw(recordsName);
                window.draw(recordsScore);
                y += 27;
                if (d > 9) {
                    break;
                }
            }
            y = 130;
        }
        
        window.draw(MenuText);
        window.draw(KeyText);
        window.draw(RecText);
        
    };
    
    auto inputName = [&]()
    {
        String playerInput;
        Text playerText("", font, 35);
        playerText.setColor(sf::Color::Yellow);
        playerText.setPosition(150, 210);
        bool done = false;
        
        Text inputText("Enter your name", font, 40);
        inputText.setColor(sf::Color::Red);
        inputText.setPosition(120, 150);
        
        Event event;
        while (!done)
        {
            window.clear(Color(40, 40, 40));
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::TextEntered)
                {
                    playerInput +=event.text.unicode;
                    playerText.setString(playerInput);
                }
                if (event.key.code == Keyboard::BackSpace)
                {
                    if (playerInput.getSize() > 0)
                    {
                        playerInput.erase(playerInput.getSize() - 1,1);
                    }
                }
                if (event.type == Event::KeyPressed)
                {
                    if (event.key.code == Keyboard::Enter)
                    {
                        done = true;
                    }
                }
            }
            window.draw(playerText);
            window.draw(inputText);
            window.display();
        }
        username = playerInput;
        
        
    };
    
    auto game = [&]()
    {
        while(!GameOver)
        {
                static float prev = clock.getElapsedTime().asSeconds();
                if (clock.getElapsedTime().asSeconds() - prev >= speed)
                {
                    prev = clock.getElapsedTime().asSeconds();
                    go_down();
                }
                Event e;
                while (window.pollEvent(e))
                {
                    if (e.type == Event::Closed) window.close();
                    if (e.type == Event::KeyPressed)
                    {
                        if (e.key.code == Keyboard::Escape)
                        {
                            GameOver = true;
                        }
                        if (e.key.code == Keyboard::Left)
                        {
                            cx--;
                            if (check_block() == false) cx++;
                        }
                        else if (e.key.code == Keyboard::Right)
                        {
                            cx++;
                            if (check_block() == false) cx--;
                        }
                        else if (e.key.code == Keyboard::Down)
                        {
                            go_down();
                        }
                        else if (e.key.code == Keyboard::Up)
                        {
                            rotate();
                            if (check_block() == false) { rotate(), rotate(), rotate(); }
                        }
                        else if (e.key.code == Keyboard::Space)
                        {
                            while (go_down() == true);
                        }
                    }
                }
                window.clear(Color(40, 40, 40));
                
                draw_world();
                draw_text();
                draw_block();
                
                window.display();
        }
    };
    
    while (window.isOpen())   // Главный цикл игры
    {
        window.clear(Color(40, 40, 40));
        menu();
        GameOver = false;
        clear();
        Event e;
        while (window.pollEvent(e))   // Обработка событий
        {
            if (e.type == Event::Closed) window.close();
            if (e.type == Event::KeyPressed)
            {
                if (e.key.code == Keyboard::Escape)  // Выход из игры
                {
                    window.close();
                }
                else if (e.key.code == Keyboard::Enter)   // Запуск игры
                {
                    inputName();
                    game();
                }
            }
        }
        window.display();
    }
    
    return 0;
}
