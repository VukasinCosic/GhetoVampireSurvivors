#include "Game.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>

Game::Game(const std::string& config)
{
	init(config);
}

void Game::init(const std::string& path)
{
	int winWidth, winHeight, frameLimit, winMode;
	int fontSize, fontR, fontG, fontB;
	std::string fontFile;
	std::ifstream fin(path);
	while (!fin.eof())
	{
		std::string filePointer;
		fin >> filePointer;
		if (filePointer == "Window")
		{
			fin >> winWidth >> winHeight >> frameLimit >> winMode;
		}

		if (filePointer == "Font")
		{
			fin >> fontFile >> fontSize >> fontR >> fontG >> fontB;
			m_font.loadFromFile(fontFile);
			m_text.setFont(m_font);
			m_text.setCharacterSize(fontSize);
			m_text.setFillColor(sf::Color(fontR, fontG, fontB));
		}

		if (filePointer == "Player")
		{
			fin >> m_playerConfig.SR >> m_playerConfig.CR >> m_playerConfig.FR >> m_playerConfig.FG
				>> m_playerConfig.FB >> m_playerConfig.OR >> m_playerConfig.OG >> m_playerConfig.OB
				>> m_playerConfig.OT >> m_playerConfig.V >> m_playerConfig.S;
		}

		if (filePointer == "Enemy")
		{
			fin >> m_enemyConfing.SR >> m_enemyConfing.CR >> m_enemyConfing.OR >> m_enemyConfing.OG
				>> m_enemyConfing.OB >> m_enemyConfing.OT >> m_enemyConfing.VMIN >> m_enemyConfing.VMAX
				>> m_enemyConfing.L >> m_enemyConfing.SI >> m_enemyConfing.SMIN >> m_enemyConfing.SMAX;
		}

		if (filePointer == "Bullet")
		{
			fin >> m_bulletConfig.SR >> m_bulletConfig.CR >> m_bulletConfig.FR >> m_bulletConfig.FG
				>> m_bulletConfig.FB >> m_bulletConfig.OR >> m_bulletConfig.OG >> m_bulletConfig.OB
				>> m_bulletConfig.OT >> m_bulletConfig.V >> m_bulletConfig.L >> m_bulletConfig.S;
		}
	}
	m_window.create(sf::VideoMode(winWidth, winHeight), "GhetoVampireSurvivors");
	m_window.setFramerateLimit(frameLimit);

	spawnPlayer();
}

void Game::run()
{
	srand(time(NULL));
	while (m_running)
	{
		m_entities.update();

		if (!m_paused)
		{
			sEnemySpawner();
			sMovement();
			sCollision();
			sLifespan();
		}

		sUserInput();
		sRender();

		if (!m_paused) 
		{
			m_currentFrame++;
		}
	}
}

void Game::setPaused()
{
	m_paused = !m_paused;
}

void Game::spawnPlayer()
{
	auto entity = m_entities.addEntity("player");

	entity->cTransform = std::make_shared<CTransform>(Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f), Vec2(0.0f, 0.0f), 0.0f);

	entity->cShape = std::make_shared<CShape>(m_playerConfig.SR, m_playerConfig.V, sf::Color(m_playerConfig.FR, m_playerConfig.FG, m_playerConfig.FB),
		sf::Color(m_playerConfig.OR, m_playerConfig.OG, m_playerConfig.OB), m_playerConfig.OT);

	entity->cCollision = std::make_shared<CCollision>(m_playerConfig.CR);

	entity->cInput = std::make_shared<CInput>();

	entity->cScore = std::make_shared<CScore>(0);

	m_player = entity;

}

void Game::spawnEnemy()
{
	int enemyVertices = (rand() % (m_enemyConfing.VMAX - m_enemyConfing.VMIN)) + m_enemyConfing.VMIN;
	int enemySpeed = (rand() % (int)(m_enemyConfing.SMAX - m_enemyConfing.SMIN)) + m_enemyConfing.SMIN;
	Vec2 enemyV(0, 0);

	float xStarting, yStarting, playerEnemyDistance;
	Vec2 startingPosition(0, 0);
	int startingSide = rand() % 4;
	switch (startingSide)
	{
	case (0):
		startingPosition.x = rand() % m_window.getSize().x;
		startingPosition.y = 0 - m_enemyConfing.SR;
		break;
	case (1):
		startingPosition.x = 0 - m_enemyConfing.SR;
		startingPosition.y = rand() % m_window.getSize().y;
		break;
	case (2):
		startingPosition.x = rand() % m_window.getSize().x;
		startingPosition.y = m_window.getSize().y + m_enemyConfing.SR;
		break;
	case (3):
		startingPosition.x = m_window.getSize().x + m_enemyConfing.SR;
		startingPosition.y = rand() % m_window.getSize().y;
		break;
	default:
		break;
	}

	enemyV = m_player->cTransform->pos - startingPosition;
	float enemyLen = sqrt(enemyV.x * enemyV.x + enemyV.y * enemyV.y);
	enemyV = enemyV / enemyLen;
	enemyV *= enemySpeed;

	auto e = m_entities.addEntity("enemy");
	// read enemy params from config
	e->cTransform = std::make_shared<CTransform>(startingPosition, enemyV, 0.0f);

	e->cShape = std::make_shared<CShape>(m_enemyConfing.SR, enemyVertices, sf::Color(rand() % 255, rand() % 255, rand() % 255),
		sf::Color(m_enemyConfing.OR, m_enemyConfing.OG, m_enemyConfing.OB), m_enemyConfing.OT);

	e->cCollision = std::make_shared<CCollision>(m_enemyConfing.CR);

	e->cUpdateTime = std::make_shared<CUpdateTime>(0);

	e->cSpeed = std::make_shared<CSpeed>(enemySpeed);

	m_lastEnemySpawnTime = m_currentFrame;
	
}

void Game::spawnSmallEnemies(std::shared_ptr<Entity> e)
{
	// TODO: spawn small enemies where the big one is destroyed
	float angles = 360.0f / e->cShape->circle.getPointCount();
	for (int i = 0; i < e->cShape->circle.getPointCount(); i++)
	{
		auto se = m_entities.addEntity("smallEnemy");		
		float xVelocity = cos(i * angles * 3.14159265 / 180.0f);
		float yVelocity = sin(i * angles * 3.14159265 / 180.0f);
		se ->cTransform = std::make_shared<CTransform>(Vec2(e->cTransform->pos.x + 10 * xVelocity, e->cTransform->pos.y + 10 * yVelocity), Vec2(xVelocity,yVelocity), 0.0f);
		se->cShape = std::make_shared<CShape>(m_enemyConfing.SR / 2.0f, e->cShape->circle.getPointCount(), sf::Color(e->cShape->circle.getFillColor()),
			sf::Color(e->cShape->circle.getOutlineColor()), e->cShape->circle.getOutlineThickness() / 2.0f);
		se->cCollision = std::make_shared<CCollision>(m_enemyConfing.CR / 2.0f);
		se->cLifespan = std::make_shared<CLifespan>(m_enemyConfing.L);
	}
}

void Game::spawnBullet(std::shared_ptr<Entity> entity, const Vec2& target)
{
	Vec2 playerPos(entity->cTransform->pos.x, entity->cTransform->pos.y);
	Vec2 bulletVelocity = target - playerPos;
	float bulletLen = sqrt(bulletVelocity.x * bulletVelocity.x + bulletVelocity.y * bulletVelocity.y);
	bulletVelocity = bulletVelocity / bulletLen;
	bulletVelocity *= m_bulletConfig.S;
	auto e = m_entities.addEntity("bullet");

	e->cTransform = std::make_shared<CTransform>(Vec2(entity->cTransform->pos.x, entity->cTransform->pos.y), bulletVelocity, 0.0f);

	e->cShape = std::make_shared<CShape>(m_bulletConfig.SR, m_bulletConfig.V, sf::Color(m_bulletConfig.FR, m_bulletConfig.FG, m_bulletConfig.FB),
		sf::Color(m_bulletConfig.OR, m_bulletConfig.OG, m_bulletConfig.OB), m_bulletConfig.OT);

	e->cCollision = std::make_shared<CCollision>(m_bulletConfig.CR);

	e->cLifespan = std::make_shared<CLifespan>(m_bulletConfig.L);
}

void Game::spawnSpecialWeapon(std::shared_ptr<Entity> entity)
{
	// triple shot?
}

void Game::sMovement()
{
	m_player->cTransform->velocity.x = 0;
	m_player->cTransform->velocity.y = 0;
	if (m_player->cInput->up && (m_player->cTransform->pos.y - m_player->cCollision->radius) > 0)
	{
		m_player->cTransform->velocity.y = -m_playerConfig.S;
	}
	if (m_player->cInput->down && (m_player->cTransform->pos.y + m_player->cCollision->radius) < m_window.getSize().y)
	{
		m_player->cTransform->velocity.y = m_playerConfig.S;
	}
	if (m_player->cInput->left && (m_player->cTransform->pos.x - m_player->cCollision->radius) > 0)
	{
		m_player->cTransform->velocity.x = -m_playerConfig.S;
	}
	if (m_player->cInput->right && (m_player->cTransform->pos.x + m_player->cCollision->radius) < m_window.getSize().x)
	{
		m_player->cTransform->velocity.x = m_playerConfig.S;
	}

	for (auto e : m_entities.getEntities())
	{
		if (e->cUpdateTime && (m_currentFrame - e->cUpdateTime->updateInterval) >= m_enemyUpdateFrame)
		{
			Vec2 eV(0, 0);
			eV = m_player->cTransform->pos - e->cTransform->pos;
			float eL = sqrt(eV.x * eV.x + eV.y * eV.y);
			eV = eV / eL;
			eV *= e->cSpeed->speed;
			e->cTransform->velocity = eV;
			e->cUpdateTime->updateInterval = m_currentFrame;
		}
		e->cTransform->pos.x += e->cTransform->velocity.x;
		e->cTransform->pos.y += e->cTransform->velocity.y;
	}

}

void Game::sLifespan()
{
	for (auto e : m_entities.getEntities())
	{
		if (e->cLifespan)
		{
			e->cLifespan->remaining--;
			if (e->cLifespan->remaining < 1)
			{
				e->destroy();
			}
		}
	}
}

void Game::sRender()
{
	m_window.clear();

	for (auto e : m_entities.getEntities())
	{
		e->cShape->circle.setPosition(e->cTransform->pos.x, e->cTransform->pos.y);
		e->cTransform->angle += 1.0f;
		e->cShape->circle.setRotation(e->cTransform->angle);
		if (e->cLifespan)
		{
			int alpha = 255 * e->cLifespan->remaining / e->cLifespan->total;
			sf::Color smallColor = e->cShape->circle.getFillColor();
			int smallColorRed = smallColor.r;
			int smallColorGreen = smallColor.g;
			int smallColorBlue = smallColor.b;
			e->cShape->circle.setFillColor(sf::Color(smallColorRed, smallColorGreen, smallColorBlue, alpha));
		}

		m_window.draw(e->cShape->circle);

	}
	std::string tempScore = "Score: ";
	tempScore += std::to_string(m_player->cScore->score);
	m_text.setString(tempScore);
	m_window.draw(m_text);
	m_window.display();
}

void Game::sEnemySpawner()
{
	if (m_currentFrame - m_lastEnemySpawnTime >= m_enemyConfing.SI)
	{
		spawnEnemy();
	}	
}

void Game::sCollision()
{
	// enemy collision for bouncing off walls (not needed in this version)
	//for (auto e : m_entities.getEntities("enemy"))
	//{
	//	if (e->cTransform->pos.x + e->cCollision->radius > m_window.getSize().x || e->cTransform->pos.x - e->cCollision->radius < 0)
	//	{
	//		e->cTransform->velocity.x *= -1;
	//	}
	//	if (e->cTransform->pos.y + e->cCollision->radius > m_window.getSize().y || e->cTransform->pos.y - e->cCollision->radius < 0)
	//	{
	//		e->cTransform->velocity.y *= -1;
	//	}
	//}
	
	// small enemy collision for bouncing off walls
	for (auto e : m_entities.getEntities("smallEnemy"))
	{
		if (e->cTransform->pos.x + e->cCollision->radius > m_window.getSize().x || e->cTransform->pos.x - e->cCollision->radius < 0)
		{
			e->cTransform->velocity.x *= -1;
		}
		if (e->cTransform->pos.y + e->cCollision->radius > m_window.getSize().y || e->cTransform->pos.y - e->cCollision->radius < 0)
		{
			e->cTransform->velocity.y *= -1;
		}
	}

	// enemy vs bullet collision
	for (auto e : m_entities.getEntities("enemy"))
	{
		for (auto b : m_entities.getEntities("bullet"))
		{
			float distance, radii;
			radii = e->cCollision->radius + b->cCollision->radius;
			radii *= radii;
			distance = (e->cTransform->pos.x - b->cTransform->pos.x) * (e->cTransform->pos.x - b->cTransform->pos.x) +
				(e->cTransform->pos.y - b->cTransform->pos.y) * (e->cTransform->pos.y - b->cTransform->pos.y);
			if (radii > distance)
			{
				b->destroy();
				e->destroy();
				m_player->cScore->score += 100 * e->cShape->circle.getPointCount();
				spawnSmallEnemies(e);
			}
		}
	}

	// small enemy vs bullet collision
	for (auto e : m_entities.getEntities("smallEnemy"))
	{
		for (auto b : m_entities.getEntities("bullet"))
		{
			float distance, radii;
			radii = e->cCollision->radius + b->cCollision->radius;
			radii *= radii;
			distance = (e->cTransform->pos.x - b->cTransform->pos.x) * (e->cTransform->pos.x - b->cTransform->pos.x) +
				(e->cTransform->pos.y - b->cTransform->pos.y) * (e->cTransform->pos.y - b->cTransform->pos.y);
			if (radii > distance)
			{
				b->destroy();
				e->destroy();
				m_player->cScore->score += 200 * e->cShape->circle.getPointCount();
			}
		}
	}

	// enemy vs player collision
	for (auto e : m_entities.getEntities("enemy"))
	{
		for (auto p : m_entities.getEntities("player"))
		{
			float distance,radii;
			radii = e->cCollision->radius + p->cCollision->radius;
			radii *= radii;
			distance = (e->cTransform->pos.x - p->cTransform->pos.x) * (e->cTransform->pos.x - p->cTransform->pos.x) +
				(e->cTransform->pos.y - p->cTransform->pos.y) * (e->cTransform->pos.y - p->cTransform->pos.y);

			if (radii > distance && (p->cTransform->pos == Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f)))
			{
				p->cTransform->pos = Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
				for (auto en : m_entities.getEntities("enemy"))
				{
					en->destroy();
				}
				p->cScore->score = 0;
			}

			if (radii > distance)
			{
				p->cTransform->pos = Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
				e->destroy();
				spawnSmallEnemies(e);
			}

		}
	}

	// small enemy vs player
	for (auto se : m_entities.getEntities("smallEnemy"))
	{
		for (auto p : m_entities.getEntities("player"))
		{
			float distance, radii;
			radii = se->cCollision->radius + p->cCollision->radius;
			radii *= radii;
			distance = (se->cTransform->pos.x - p->cTransform->pos.x) * (se->cTransform->pos.x - p->cTransform->pos.x) +
				(se->cTransform->pos.y - p->cTransform->pos.y) * (se->cTransform->pos.y - p->cTransform->pos.y);
			if (radii > distance)
			{
				p->cTransform->pos = Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
				se->destroy();
			}
		}
	}
}

void Game::sUserInput()
{
	sf::Event event;

	while (m_window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			m_running = false;
		}

		if (event.type == sf::Event::KeyPressed)
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				m_player->cInput->up = true;
				break;
			case sf::Keyboard::S:
				m_player->cInput->down = true;
				break;
			case sf::Keyboard::A:
				m_player->cInput->left = true;
				break;
			case sf::Keyboard::D:
				m_player->cInput->right = true;
				break;
			case sf::Keyboard::P:
				setPaused();
				break;
			default:
				break;
			}
		}

		if (event.type == sf::Event::KeyReleased)
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				m_player->cInput->up = false;
				break;
			case sf::Keyboard::S:
				m_player->cInput->down = false;
				break;
			case sf::Keyboard::A:
				m_player->cInput->left = false;
				break;
			case sf::Keyboard::D:
				m_player->cInput->right = false;
				break;
			default:
				break;
			}
		}

		if (event.type == sf::Event::MouseButtonPressed)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				if (!m_paused)
				{
					spawnBullet(m_player, Vec2(event.mouseButton.x, event.mouseButton.y));
				}
			}
		}
	}
}