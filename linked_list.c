#include <stdio.h>
#include <stdlib.h>

// 링크드 리스트 노드 구조체 정의
struct Node
{
  int data;
  struct Node *next;
};

// 새로운 노드 생성 함수
struct Node *createNode(int data)
{
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  newNode->data = data;
  newNode->next = NULL;
  return newNode;
}

// 링크드 리스트 출력 함수
void printList(struct Node *head)
{
  struct Node *current = head;
  while (current != NULL)
  {
    printf("%d ", current->data);
    current = current->next;
  }
  printf("\n");
}

int main()
{
  // 노드 생성
  struct Node *head = createNode(1);
  struct Node *second = createNode(2);
  struct Node *third = createNode(3);

  // 노드 연결
  head->next = second;
  second->next = third;

  // 링크드 리스트 출력
  printf("Linked List: ");
  printList(head);

  return 0;
}
