## masterFunction() 

Contine logica procesului Master ce
deschide fisierul de input din 4 threaduri ce primesc functia
f(). Acestea citesc fisierul in paralel si il trimit paragraf cu paragraf.
Succesiv paragrafele procesate sunt primite si bagate intr-un vector ce pastreaza
ordinea lor si apoi cu un thread vectorul este parcurs
si transferat in fisierul de out.

## workerFunction() 

Porneste initial un thread pentru primirea datelor prin
	-> receiver() asteapta paragrafe pana la primirea stringului
	"opreste citire". Pentru fiecare paragraf porneste numarul necesar
	de threaduri dupa cum se precizeaza in enunt si la final uneste
	liniile si trimite paragraful obtinut la Master.
	-> g() proceseaza liniile in functie de genul paragrafului.


