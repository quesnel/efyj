efyj's todo list
----------------

- Problem updates:
  - Add time limit (4 days, 4096 hours?)
  - Add kappa limit (==1 or when kappa decrease)
- GUI: Tdds a simple QT GUI for Windows users.
- ABI: providEs a Ltable API:
  - pimpl for complex class (context?)
  - remove dead code
  - use exception and noexcept
  - simplify algorithms


### Example prediction (french)

Le choix du critère pour la validation croisée.

1. Prendre un site-année et calculer la valeur simulée par le modèle
   sur ce site après avoir ajusté la première ligne du premier tableau
   d'IPSIM sur l'ensemble des autres sites-années, sauf ceux
   correspondant +au même site ou à la même année. Stocker le couple
   observé-simulé dans une matrice de confusion.
2. Répéter l'opération pour l'ensemble des sites années. On obtient
   donc une matrice de confusion ayant le même nombre d'éléments qu'il
   y a de sites-années dans la base de données.
3. Calculer le critère de Kappa pondéré quadratiquement (K\_QW) sur
   cette matrice de confusion (pour la première ligne du premier
   tableau) et stocker le résultat.
4. Répéter l'opération pour l'ensemble des lignes de tous les tableaux
   du modèle IPSIM considéré.
5. On stockera la valeur de K\_QW la plus élevée et la ligne
   correspondante, ainsi que la matrice de confusion associée.
6. Répéter l'opération en considérant cette fois-ci l'ensemble des
   combinaisons de 2 lignes ajustées simultanément (pour 150 lignes,
   cela fait 11175 couples de lignes possibles ; de mémoire c'est
   l'ordre de +grandeur du nombre de lignes de tous les tableaux du
   modèle IPSIM-Wheat-BR, mais c'est à corriger par la véritable
   valeur).
7. Répéter en augmentant à 3, 4, 5, ... n lignes ajustées
   simultanément jusqu'à ce que i) le K\_QW décroisse ou que ii) le
   calcul ne soit plus faisable en termes de temps d'exécution (pour
   avoir un ordre de +grandeur, si l'on vise de tester 1, 2, 3, 4, 5,
   6, 7, 8, 9, 10 lignes ajustées simultanément, cela fait 1.25807
   10^15 matrices de confusion à calculer si le modèle comporte 150
   lignes, et chaque matrice +nécessite elle-même plus de simulations
   que les 1740 simulations figurant dans la matrice puisqu'il faut
   compter également celles nécessaires aux ajustements).

L'idéal serait d'avoir une sortie de la forme :

    | ------------------ | ---- | ------------------- | -------------------- |
    | nb lignes ajustées | K_QW | Lignes ajustées     | Matrice de confusion |
    | ------------------ | ---- | ------------------- | -------------------- |
    | 1                  | 0.45 | 8                   | M1                   |
    | 2                  | 0.73 | 8, 23               | M2                   |
    | 3                  | 0.82 | 7, 23, 120          | M3                   |
    | 4                  | 0.87 | 3, 12, 13, 119      | M4                   |
    | 5                  | 0.85 | 8, 12, 13, 112, 148 | M5                   |
    |                    |      |                     |                      |
    |------------------- | ---- | ------------------- | -------------------- |
