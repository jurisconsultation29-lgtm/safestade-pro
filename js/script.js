// Fonction de sauvegarde et d'envoi du webhook vers Abafusion
function saveAbaWebhook() {
    // 1. Récupérer l'URL saisie dans l'input (correspondant à l'id="abawebhook" dans votre HTML)
    const inputField = document.getElementById('abawebhook');
    const resultNote = document.getElementById('abaResult');

    if (!inputField || !inputField.value) {
        if (resultNote) {
            resultNote.innerText = "Erreur : Veuillez entrer une URL de webhook valide.";
            resultNote.style.color = "orange";
        }
        return;
    }

    const webhookUrl = inputField.value;

    // 2. Sauvegarder l'URL dans le stockage local du navigateur
    localStorage.setItem('savedAbaWebhook', webhookUrl);
    if (resultNote) {
        resultNote.innerText = "Enregistrement en cours et test d'envoi...";
        resultNote.style.color = "blue";
    }

    // 3. Préparer la payload de données à envoyer vers Abafusion
    const payload = {
        source: "SafeStade-Interface",
        action: "manual_sync",
        timestamp: new Date().toISOString(),
        autoSync: document.getElementById('abaAutoSync') ? document.getElementById('abaAutoSync').checked : false
    };

    // 4. Exécuter l'appel réseau POST vers le webhook
    fetch(webhookUrl, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(payload)
    })
    .then(response => {
        if (response.ok) {
            if (resultNote) {
                resultNote.innerText = "Succès : Synchronisation Abafusion réussie !";
                resultNote.style.color = "green";
            }
        } else {
            if (resultNote) {
                resultNote.innerText = "Échec : Le serveur a répondu avec le statut " + response.status;
                resultNote.style.color = "red";
            }
        }
    })
    .catch(error => {
        if (resultNote) {
            resultNote.innerText = "Échec de connexion (CORS ou Réseau) : " + error.message;
            resultNote.style.color = "red";
        }
    });
}

// Restauration automatique de l'URL enregistrée au chargement de la page
window.addEventListener('DOMContentLoaded', () => {
    const savedUrl = localStorage.getItem('savedAbaWebhook');
    const inputField = document.getElementById('abawebhook');
    if (savedUrl && inputField) {
        inputField.value = savedUrl;
    }
});