#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include <functional>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <Windows.h>

using vec2     = sf::Vector2f;
using button_f = std::function<void(bool&)>;

constexpr unsigned    width                = 512;
constexpr unsigned    height               = 512;
constexpr unsigned    char_point           = 256;
constexpr float       char_size            = 0.5f;
constexpr char        char_0               = 'A';
constexpr char        char_1               = 'S';
constexpr const char* font_path            = "Resources\\consola.ttf";
const     sf::Color   button_color_default = sf::Color(64, 64, 64 , 255);
const     sf::Color   button_color_pushed  = sf::Color(64, 64, 255, 255);
const     vec2        kps_pos              = vec2(width * 0.5f, height * 0.85f);
const     vec2        kps_size             = vec2(width * 0.5f, width * 0.5f);
const     vec2        key0_pos             = vec2(width * 0.28f, height * 0.3f);
const     vec2        key1_pos             = vec2(width * 0.72f, height * 0.3f);
const     vec2        key_size             = vec2(width * 0.4f, width * 0.4f);

struct Resources
{
	static std::unique_ptr<sf::Font> font;

	static inline void init()
	{
		font = std::make_unique<sf::Font>();
		font->loadFromFile(font_path);
	}

	static inline void centralize_text(sf::Text& text, bool ignore_y = false)
	{
		auto rectl = text.getLocalBounds();

		if (ignore_y)
			text.setOrigin(vec2(rectl.left + rectl.width * 0.5f, text.getOrigin().y));
		else
			text.setOrigin(vec2(rectl.left + rectl.width * 0.5f, rectl.top + rectl.height * 0.5f));
	}

	static inline void setup_text(sf::Text& text, const vec2& size)
	{
		auto rectg = text.getGlobalBounds();
		float adj = std::min(size.x / rectg.width, size.y / rectg.height);
		text.setScale(vec2(adj, adj));
		centralize_text(text);
	}
};
std::unique_ptr<sf::Font> Resources::font;

class Button
{
	sf::RectangleShape body;
	sf::Text           key;
	sf::Text           counter;
	button_f           func;
	vec2               size;
	bool               pushed_prev = false;
	bool               onpush      = false;
	unsigned           key_count   = 0;

public:

	explicit Button(const vec2& pos, const vec2& size, char character, button_f&& func)
		: size(size), func(std::move(func))
	{
		/* body setup */
		body.setFillColor(button_color_default);
		body.setSize(size);
		body.setOrigin(vec2(size.x * 0.5f, size.y * 0.5f));
		body.setPosition(pos);

		/* key setup */
		key.setFont(*Resources::font);
		key.setCharacterSize(char_point);
		key.setString(character);
		key.setPosition(pos);
		Resources::setup_text(key, size * char_size);

		/* counter setup */
		counter.setFont(*Resources::font);
		counter.setCharacterSize(char_point);
		counter.setString(std::to_string(key_count));
		counter.setPosition(vec2(pos.x, pos.y + size.y * 0.8f));
		Resources::setup_text(counter, size * char_size * 0.5f);
	}

	void draw(sf::RenderWindow& window)
	{
		bool pushed;
		func(pushed);
		
		onpush = pushed && !pushed_prev;
		
		if (onpush)
		{
			++key_count;
			counter.setString(std::to_string(key_count));
			Resources::centralize_text(counter, true);
		}

		pushed_prev = pushed;

		body.setFillColor(pushed ? button_color_pushed : button_color_default);

		window.draw(body);
		window.draw(key);
		window.draw(counter);
	}

	inline bool get_onpush() { return onpush; }
};

struct Counter
{
	static std::vector<std::unique_ptr<Button>>                        buttons;
	static std::vector<std::chrono::high_resolution_clock::time_point> log;
	static int                                                         kps;
	static sf::Text                                                    kps_text;

private:

	static inline void set_kps()
	{
		kps_text.setString("KPS: " + std::to_string(kps));
	}

public:

	static inline void init()
	{
		buttons.resize(2);

		auto a = std::make_unique<Button>(key0_pos, key_size, char_0, [](bool& pushed) { pushed = GetAsyncKeyState(char_0); });
		auto s = std::make_unique<Button>(key1_pos, key_size, char_1, [](bool& pushed) { pushed = GetAsyncKeyState(char_1); });

		buttons[0] = std::move(a);
		buttons[1] = std::move(s);

		kps = 0;

		kps_text.setFont(*Resources::font);
		kps_text.setCharacterSize(char_point);
		kps_text.setPosition(kps_pos);
		set_kps();
		Resources::setup_text(kps_text, kps_size);
	}

	static inline void draw(sf::RenderWindow& window)
	{
		auto now = std::chrono::high_resolution_clock::now();

		for (auto& b : buttons)
		{
			if (b->get_onpush())
			{
				log.push_back(now);
			}

			b->draw(window);
		}

		int kps_temp = 0;
		for (auto itr = log.begin(); itr != log.end(); )
		{
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - *itr).count() > 1000)
				itr = log.erase(itr);
			else
			{
				++kps_temp;
				++itr;
			}
		}
		
		kps = kps_temp;
		set_kps();
		Resources::centralize_text(kps_text, true);

		window.draw(kps_text);
	}
};
std::vector<std::unique_ptr<Button>>                        Counter::buttons;
std::vector<std::chrono::high_resolution_clock::time_point> Counter::log;
sf::Text                                                    Counter::kps_text;
int                                                         Counter::kps;

int main()
{
	Resources::init();
	Counter::init();

	auto window = std::make_unique<sf::RenderWindow>(
		sf::VideoMode(width, height),
		"KeyCounter"
	);

	window->setFramerateLimit(60);

	while (window->isOpen())
	{
		sf::Event event;

		while (window->pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window->close();
				break;
			}
		}

		window->clear();

		Counter::draw(*window);

		window->display();
	}

	return 0;
}