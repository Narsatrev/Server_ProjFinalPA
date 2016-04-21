if [ -z "$1" ]; then    
    echo "Escribe un mensaje para el commit!"
    exit 1
fi

git add .
git commit -m "$1"
git push origin master
