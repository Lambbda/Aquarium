# Aquarium
Курсовая работа по программированию, С++. Игра "Аквариум".

Техническое задание:

1.	Использовать ООП (Объектно-Ориентированное Программирование) и методы из библиотеки STL (Standard Template Library).  
2.	Определить объект TFish - аквариумная рыбка. Рыбка имеет координаты, скорость, размер, цвет, направление движения. Методами объекта являются:  
•	Init - устанавливает значения полей объекта и рисует рыбу на экране методом Draw.  
•	Draw - рисует рыбу в виде уголка, с острием в точке Coord и направленного острием по ходу движения рыбы.  
•	Look - проверяет несколько точек на линии движения рыбы. Если хоть одна из них отличается по цвету от воды, возвращается её цвет и расстояние до рыбы. Run - перемещает рыбу в текущем направлении на расстояние, зависящее от текущей скорости рыбы. Иногда случайным образом меняет направление движения рыбы. Если рыба видит препятствие, направление движения меняется, пока препятствие не исчезнет из поля зрения рыбы.  
3.	Определить объект Taquarium, который является местом обитания рыб. Он представляет собой область экрана, наполненную водой. Рыбы живут в аквариуме, поэтому экземпляры объекта Tfish должны быть полями объекта Taquarium. Методы:  
•	Init - включает графический режим, заполняетаквариум водой, скалами и рыбами.  
•	Run - организует бесконечный цикл, в котором выполняется метод Run всех обитателей аквариума.  
•	Done - выключает графический режим.  
4.	Определить два объекта Tpike и Tkarp , которые наследуют объект Tfish. Оба они отличаются от Tfish тем, что по разному изображают себя на экране: Tpike - в виде зеленой стрелки, а Tkarp – в виде красного треугольника. Воспользуйтесь виртуальными методами. Для этого вернитесь к определению Tfish и откорректируйте его, сделав Draw пустым и виртуальным. Объедините карпов и щук в две стаи. Стая - это связанный список рыб в динамической памяти. Для связи добавить в объекты Tpike и Tkarp поле Next - указатель на следующую рыбу в стае. Сделайте аквариум не владельцем рыб, а двух стай и позвольте пользователю пополнять стаи, вводя рыб с клавиатуры.  
5.	Позволить щукам проявить свой дурной характер и поедать карпов, как только они их увидят. Здесь возникнет проблема - установить, какого именно карпа видит щука. Она решается путем просмотра всей стаи карпов и поиска того, чьи координаты близки к координатам данной щуки. Найденный карп удаляется из стаи.
