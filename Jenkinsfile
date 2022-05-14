pipeline {
    agent {
        docker { image 'fare-docker-reg.dock.studios:5000/docker-images/cpp-static-code-analysis:latest' }
    }
    stages {
        stage('build') {
            steps {
                sh 'rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake'
                sh 'cmake .'
                sh 'make clean all'
                stash includes: 'GameboyEmulator', name: 'app'
            }
        }
        stage('Test 01-special') {
            agent {
                docker { image 'fare-docker-reg.dock.studios:5000/docker-images/screenshot-tool:latest' }
            }
            steps {
              unstash 'app'
              sh 'bash ./tests/system/01-special.sh'
            }
        }
    }
}
