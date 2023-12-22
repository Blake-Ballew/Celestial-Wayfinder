#include "OLED_Content.h"

Adafruit_SSD1306 *OLED_Content::display = nullptr;

void OLED_Content::drawBatteryIcon(size_t x, size_t y)
{
    display->drawRect(x, y, 12, 8, WHITE);
    display->fillRect(x + 12, y + 2, 2, 4, WHITE);
}

void OLED_Content::drawBatteryIcon(size_t x, size_t y, uint8_t percentage)
{
    if (percentage > 100)
    {
        percentage = 100;
    }
    display->drawRect(x, y, 12, 8, WHITE);
    display->fillRect(x + 1, y, percentage / 10, 8, WHITE);
    display->fillRect(x + 12, y + 2, 2, 4, WHITE);
}

void OLED_Content::drawMessageIcon(size_t x, size_t y)
{
    display->fillRect(x, y, 12, 8, WHITE);
    display->drawLine(x, y, x + 5, y + 3, BLACK);
    display->drawLine(x + 6, y + 3, x + 11, y, BLACK);
}

void OLED_Content::drawBellIcon(size_t x, size_t y, bool isSilent)
{
    display->fillCircle(x + 5, y + 2, 3, WHITE);
    display->fillRect(x + 2, y + 2, 7, 6, WHITE);
    display->fillTriangle(x, y + 7, x + 10, y + 7, x + 5, y + 2, WHITE);
    if (isSilent)
    {
        display->drawLine(x, y + 5, x + 8, y + 1, BLACK);
        display->drawLine(x, y + 6, x + 8, y + 2, BLACK);
    }
}

Content_Node::Content_Node(uint32_t resourceID, const char *nodeText, uint8_t textLength)
{
    this->resourceID = resourceID;
    strncpy(this->nodeText, nodeText, NODE_TEXT_MAX);
    this->nodeText[NODE_TEXT_MAX] = '\0'; // Make sure string is null terminated
    this->textLength = textLength;
}

Content_Node::~Content_Node()
{
}

OLED_Content_List::OLED_Content_List(Adafruit_SSD1306 *display)
{
    this->display = display;
    this->head = NULL;
    this->current = NULL;
    this->listSize = 0;
    this->type = ContentType::LIST;
}

void OLED_Content_List::addNode(Content_Node *node)
{
    if (head == NULL)
    {
        head = node;
        head->next = head;
        head->prev = head;
        current = head;
    }
    else
    {
        node->next = head;
        node->prev = head->prev;
        head->prev->next = node;
        head->prev = node;
    }
    listSize++;
}

void OLED_Content_List::removeNode(Content_Node *node)
{
    if (node == head)
    {
        head = node->next;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    listSize--;
}

Content_Node *OLED_Content_List::getCurrentNode()
{
    return current;
}

void OLED_Content_List::encUp()
{
#if DEBUG == 1
    Serial.println("void OLED_Content::encUp()");
#endif
    current = current->prev;
    OLED_Content_List::printContent();
    display->display();
}

void OLED_Content_List::encDown()
{
#if DEBUG == 1
    Serial.println("void OLED_Content::encDown()");
#endif
    current = current->next;
    OLED_Content_List::printContent();
    display->display();
}

void OLED_Content_List::printContent()
{
    // display->drawRect(6, 8, OLED_WIDTH - 6, OLED_HEIGHT - 8, WHITE);
    display->fillRect(0, 8, OLED_WIDTH, OLED_HEIGHT - 16, BLACK);
    display->setCursor((OLED_WIDTH / 2) - ((strlen(current->nodeText) * 6 / 2)), (OLED_HEIGHT / 2) - 4);
    display->println(current->nodeText);
    display->setCursor(OLED_Content::centerTextHorizontal(1), OLED_Content::selectTextLine(1));
    display->print("^");
    display->setCursor(OLED_Content::centerTextHorizontal(1), OLED_Content::selectTextLine(4));
    display->print("v");

    display->display();
}

OLED_Content_List::~OLED_Content_List()
{
    Content_Node *temp = head;
    for (uint8_t i = 0; i < listSize; i++)
    {
        temp = temp->next;
        delete temp->prev;
    }
}