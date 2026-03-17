import org.gradle.api.GradleException
import org.gradle.api.publish.maven.MavenPublication

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("maven-publish")
}

android {
    namespace = "com.tinalux.runtime"
    compileSdk = 35

    defaultConfig {
        minSdk = 26
        consumerProguardFiles("consumer-rules.pro")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
        debug {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    sourceSets {
        getByName("main") {
            java.srcDir(projectDir.resolve("../host/src/main/kotlin"))
            manifest.srcFile("src/main/AndroidManifest.xml")
            jniLibs.srcDirs("src/main/jniLibs")
            assets.srcDirs("src/main/assets")
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}

dependencies {
}

val tinaluxGroupId = providers.gradleProperty("tinalux.groupId").orElse("com.tinalux")
val tinaluxArtifactId = providers.gradleProperty("tinalux.artifactId").orElse("tinalux-android-sdk")
val tinaluxVersion = providers.gradleProperty("tinalux.version").orElse("0.1.0-SNAPSHOT")

val verifyTinaluxNativeArtifacts by tasks.registering {
    group = "verification"
    description = "Ensures the staged Tinalux native artifacts exist before packaging the SDK."

    doLast {
        val jniLibRoot = layout.projectDirectory.dir("src/main/jniLibs").asFile
        val nativeLibraries = if (jniLibRoot.exists()) {
            jniLibRoot.walkTopDown()
                .filter { it.isFile && it.name == "libtinalux_native.so" }
                .toList()
        } else {
            emptyList()
        }

        if (nativeLibraries.isEmpty()) {
            throw GradleException(
                """
                Missing staged Tinalux native library.
                Build the Android shared library first, then stage it into this module.

                Recommended command:
                  powershell -ExecutionPolicy Bypass -File ../../scripts/build_android_native.ps1 -Abi arm64-v8a -StageToSdk
                """.trimIndent(),
            )
        }

        val assetsDir = layout.projectDirectory.dir("src/main/assets").asFile
        if (!assetsDir.resolve("icudtl.dat").exists()) {
            logger.lifecycle(
                "Optional Skia runtime asset icudtl.dat was not staged. " +
                    "If your Android configuration requires it, stage it alongside the native library."
            )
        }
    }
}

tasks.named("preBuild").configure {
    dependsOn(verifyTinaluxNativeArtifacts)
}

afterEvaluate {
    publishing {
        publications {
            create<MavenPublication>("release") {
                from(components["release"])
                groupId = tinaluxGroupId.get()
                artifactId = tinaluxArtifactId.get()
                version = tinaluxVersion.get()

                pom {
                    name.set("Tinalux Android SDK")
                    description.set("Android host/runtime SDK module for the Tinalux rendering engine.")
                }
            }
        }
    }
}

tasks.register("publishTinaluxSdkToMavenLocal") {
    group = "publishing"
    description = "Publishes the Tinalux Android SDK AAR to Maven Local."
    dependsOn("publishReleasePublicationToMavenLocal")
}
