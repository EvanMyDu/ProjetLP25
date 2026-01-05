# ProjetLP25

git add . //ajoute tout les nouveaux fichiers
git status //donne ce qui est nouveau, les modifications,...

git commit -m "le message" //fait un commit avec le message choisis

git push origin main //push le commit sur le main

git pull origin main //pull les modifications faites sur le main

make pour lancer
make clean pour supprimer les objets

./GestionRessources pour lancer le programme depuis le terminal linux

pour voir un changement d'utilisation de la RAM il faut faire un alt+tab pour changer
de fenêtre et revenir sur le terminal, sinon la ram ne veut pas s'actualiser



Rendu du 10 décembre :

&nbsp;	- la fonction search ne marche pas, on peut l'appeler et écrire mais ça ne nous donne rien quand on appui sur entrée

&nbsp;	- la partie réseau n'est pas encore implémenter

Pour que le code compile, il faut installer ssh : 

sudo get-apt update 
puis
sudo get-apt install libssh-dev

Il est possible que le dossier obj ne soit pas présent ou soit cacher lorsqu'on clone le projet, il faut donc le créer pour que le makefile puisse marcher



