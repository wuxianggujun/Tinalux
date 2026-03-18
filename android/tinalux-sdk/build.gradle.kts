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
val tinaluxNativeAbi = providers.gradleProperty("tinalux.nativeAbi").orElse("arm64-v8a")
val tinaluxNativeBuildType = providers.gradleProperty("tinalux.nativeBuildType").orElse("Release")
val tinaluxAndroidApi = providers.gradleProperty("tinalux.androidApi").orElse("26")
val tinaluxAutoBuildNative = providers.gradleProperty("tinalux.autoBuildNative").map {
    it.equals("true", ignoreCase = true)
}.orElse(false)
val tinaluxNativeBuildRoot = providers.gradleProperty("tinalux.nativeBuildRoot")
val tinaluxNativeIcuData = providers.gradleProperty("tinalux.nativeIcuData")
val tinaluxNdkPath = providers.gradleProperty("tinalux.ndkPath")
    .orElse(
        providers.environmentVariable("ANDROID_NDK_ROOT")
            .orElse(providers.environmentVariable("ANDROID_NDK_HOME"))
    )

fun File.normalizedPath(): String = canonicalFile.absolutePath
fun collectStagedNativeLibraries(jniLibRoot: File): Map<String, File> {
    if (!jniLibRoot.exists()) {
        return emptyMap()
    }

    return jniLibRoot.listFiles()
        ?.filter { it.isDirectory }
        ?.sortedBy { it.name }
        ?.mapNotNull { abiDir ->
            val library = abiDir.resolve("libtinalux_native.so")
            if (library.exists() && library.isFile) {
                abiDir.name to library
            } else {
                null
            }
        }
        ?.toMap()
        ?: emptyMap()
}

val repoRoot = layout.projectDirectory.dir("../..").asFile.canonicalFile
val sdkModuleRoot = layout.projectDirectory.asFile.canonicalFile
val supportedNativeAbis = setOf("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
val configuredNativeBuildRoot = tinaluxNativeBuildRoot.map { file(it).canonicalFile.absolutePath }
    .orElse(
        provider {
            repoRoot.resolve(
                "build-android-${tinaluxNativeAbi.get()}-${tinaluxNativeBuildType.get().lowercase()}",
            ).normalizedPath()
        }
    )
val configuredNdkPath = tinaluxNdkPath.map { file(it).canonicalFile.absolutePath }

val validateTinaluxNativeConfiguration by tasks.registering {
    group = "verification"
    description = "Validates Tinalux Android native build inputs before packaging the SDK."

    doLast {
        val requestedAbi = tinaluxNativeAbi.get()
        if (requestedAbi !in supportedNativeAbis) {
            throw GradleException(
                "Unsupported Tinalux Android ABI '$requestedAbi'. Supported values: ${supportedNativeAbis.joinToString(", ")}",
            )
        }

        val configuredBuildType = tinaluxNativeBuildType.get()
        if (configuredBuildType != "Debug" && configuredBuildType != "Release") {
            throw GradleException("Unsupported Tinalux Android build type '$configuredBuildType'. Use Debug or Release.")
        }

        val configuredApiLevel = tinaluxAndroidApi.get().toIntOrNull()
            ?: throw GradleException("Configured tinalux.androidApi must be an integer, but was '${tinaluxAndroidApi.get()}'.")
        if (configuredApiLevel < 26) {
            throw GradleException("Configured tinalux.androidApi must be >= 26, but was $configuredApiLevel.")
        }

        val icuDataPath = tinaluxNativeIcuData.orNull
        if (!icuDataPath.isNullOrBlank()) {
            val sourceIcuData = file(icuDataPath)
            if (!sourceIcuData.exists()) {
                throw GradleException("Configured ICU data file does not exist: ${sourceIcuData.normalizedPath()}")
            }
        }

        if (tinaluxAutoBuildNative.get()) {
            val ndkPath = configuredNdkPath.orNull
                ?: throw GradleException(
                    "Missing Android NDK path. Set -Ptinalux.ndkPath or ANDROID_NDK_ROOT/ANDROID_NDK_HOME.",
                )
            val toolchainFile = file(ndkPath).resolve("build/cmake/android.toolchain.cmake")
            if (!toolchainFile.exists()) {
                throw GradleException("Android toolchain file not found: ${toolchainFile.normalizedPath()}")
            }
        }
    }
}

val verifyTinaluxNativeArtifacts by tasks.registering {
    group = "verification"
    description = "Ensures the staged Tinalux native artifacts exist before packaging the SDK."

    doLast {
        val jniLibRoot = layout.projectDirectory.dir("src/main/jniLibs").asFile
        val stagedLibraries = collectStagedNativeLibraries(jniLibRoot)
        val requestedAbi = tinaluxNativeAbi.get()
        val requestedLibrary = stagedLibraries[requestedAbi]

        if (requestedLibrary == null) {
            val availableAbis = if (stagedLibraries.isEmpty()) {
                "(none)"
            } else {
                stagedLibraries.keys.joinToString(", ")
            }
            throw GradleException(
                """
                Missing staged Tinalux native library for ABI '$requestedAbi'.
                Build the Android shared library for the requested ABI first, then stage it into this module.

                Recommended command:
                  powershell -ExecutionPolicy Bypass -File ../../scripts/build_android_native.ps1 -Abi $requestedAbi -StageToSdk

                Currently staged ABIs:
                  $availableAbis
                """.trimIndent(),
            )
        }

        if (requestedLibrary.length() <= 0L) {
            throw GradleException("Staged Tinalux native library is empty: ${requestedLibrary.normalizedPath()}")
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

val describeTinaluxNativeArtifacts by tasks.registering {
    group = "help"
    description = "Prints the currently staged Tinalux Android native artifacts for diagnosis."

    doLast {
        val jniLibRoot = layout.projectDirectory.dir("src/main/jniLibs").asFile
        val stagedLibraries = collectStagedNativeLibraries(jniLibRoot)
        logger.lifecycle("Requested ABI: ${tinaluxNativeAbi.get()}")
        if (stagedLibraries.isEmpty()) {
            logger.lifecycle("Staged native libraries: (none)")
        } else {
            stagedLibraries.forEach { (abi, library) ->
                logger.lifecycle("Staged native library [$abi]: ${library.normalizedPath()}")
            }
        }

        val icuAsset = layout.projectDirectory.file("src/main/assets/icudtl.dat").asFile
        logger.lifecycle(
            "Optional ICU asset: ${if (icuAsset.exists()) icuAsset.normalizedPath() else "(not staged)"}",
        )
    }
}

val buildAndStageTinaluxNative by tasks.registering {
    group = "build"
    description = "Builds TinaluxAndroidNative with CMake and stages the result into this SDK module."

    doLast {
        val ndkPath = configuredNdkPath.orNull
            ?: throw GradleException(
                "Missing Android NDK path. Set -Ptinalux.ndkPath or ANDROID_NDK_ROOT/ANDROID_NDK_HOME.",
            )
        val toolchainFile = file(ndkPath).resolve("build/cmake/android.toolchain.cmake")
        if (!toolchainFile.exists()) {
            throw GradleException("Android toolchain file not found: ${toolchainFile.normalizedPath()}")
        }

        val buildRoot = file(configuredNativeBuildRoot.get())
        buildRoot.mkdirs()

        exec {
            workingDir = repoRoot
            commandLine(
                "cmake",
                "-S", repoRoot.normalizedPath(),
                "-B", buildRoot.normalizedPath(),
                "-G", "Ninja",
                "-DANDROID=ON",
                "-DCMAKE_TOOLCHAIN_FILE=${toolchainFile.normalizedPath()}",
                "-DANDROID_ABI=${tinaluxNativeAbi.get()}",
                "-DANDROID_PLATFORM=android-${tinaluxAndroidApi.get()}",
                "-DCMAKE_BUILD_TYPE=${tinaluxNativeBuildType.get()}",
                "-DANDROID_STL=c++_static",
                "-DTINALUX_BUILD_TESTS=OFF",
            )
        }

        exec {
            workingDir = repoRoot
            commandLine(
                "cmake",
                "--build", buildRoot.normalizedPath(),
                "--config", tinaluxNativeBuildType.get(),
                "--target", "TinaluxAndroidNative",
            )
        }

        val library = buildRoot.walkTopDown()
            .filter { it.isFile && it.name == "libtinalux_native.so" }
            .maxByOrNull { it.lastModified() }
        if (library == null) {
            throw GradleException("Failed to locate libtinalux_native.so under ${buildRoot.normalizedPath()}")
        }

        copy {
            from(library)
            into(layout.projectDirectory.dir("src/main/jniLibs/${tinaluxNativeAbi.get()}"))
            rename { "libtinalux_native.so" }
        }

        val icuDataPath = tinaluxNativeIcuData.orNull
        if (!icuDataPath.isNullOrBlank()) {
            val sourceIcuData = file(icuDataPath)
            if (!sourceIcuData.exists()) {
                throw GradleException("Configured ICU data file does not exist: ${sourceIcuData.normalizedPath()}")
            }
            copy {
                from(sourceIcuData)
                into(layout.projectDirectory.dir("src/main/assets"))
                rename { "icudtl.dat" }
            }
        }

        logger.lifecycle(
            "Staged Tinalux Android native library for ABI ${tinaluxNativeAbi.get()} into ${sdkModuleRoot.resolve("src/main/jniLibs").normalizedPath()}",
        )
    }
}

val cleanStagedTinaluxNative by tasks.registering(Delete::class) {
    group = "build"
    description = "Removes staged Android native libraries and optional runtime assets from the SDK module."
    delete(
        layout.projectDirectory.dir("src/main/jniLibs"),
        layout.projectDirectory.file("src/main/assets/icudtl.dat"),
    )
}

tasks.named("preBuild").configure {
    dependsOn(validateTinaluxNativeConfiguration)
    if (tinaluxAutoBuildNative.get()) {
        dependsOn(buildAndStageTinaluxNative)
    }
    dependsOn(verifyTinaluxNativeArtifacts)
}

buildAndStageTinaluxNative.configure {
    dependsOn(validateTinaluxNativeConfiguration)
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
