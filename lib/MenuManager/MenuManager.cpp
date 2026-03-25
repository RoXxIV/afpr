#include "MenuManager.h"

MenuManager::MenuManager(DisplayManager &display)
  : _display(display), _count(0), _current(0), _needsRedraw(true)
{
  for (int i = 0; i < MAX_PAGES; i++)
  {
    _pages[i] = nullptr;
    _data[i]  = nullptr;
  }
}

void MenuManager::addPage(PageFunc page, void *data)
{
  if (_count >= MAX_PAGES) return;
  _pages[_count] = page;
  _data[_count]  = data;
  _count++;
}

void MenuManager::next()
{
  _current = (_current + 1) % _count; // Boucle sur la première page après la dernière
  _needsRedraw = true;
}

void MenuManager::prev()
{
  _current = (_current - 1 + _count) % _count; // Boucle sur la dernière avant la première
  _needsRedraw = true;
}

void MenuManager::refresh()
{
  if (!_needsRedraw) return;
  _needsRedraw = false;

  if (_pages[_current] != nullptr)
    _pages[_current](_display, _data[_current]);
}

void MenuManager::markDirty()
{
  _needsRedraw = true;
}

int MenuManager::currentPage()
{
  return _current;
}
