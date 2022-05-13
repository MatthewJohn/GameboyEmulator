pipeline {
    agent {
        docker { image 'registry.gitlab.dockstudios.co.uk/pub/docker-images/cpp-static-code-analysis:latest:latest' }
    }
    stages {
        stage('Build') {
            steps {
                sh 'cmake .'
                sh 'make all'
            }
        }
    }
}
